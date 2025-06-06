/*
 * SPDX-FileCopyrightText: 2012-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "unikey-im.h"
#include "charset.h"
#include "inputproc.h"
#include "keycons.h"
#include "unikey-config.h"
#include "unikeyinputcontext.h"
#include "usrkeymap.h"
#include "vnconv.h"
#include "vnlexi.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcitx-config/iniparser.h>
#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx-utils/textformatflags.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/action.h>
#include <fcitx/addoninstance.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/menu.h>
#include <fcitx/statusarea.h>
#include <fcitx/text.h>
#include <fcitx/userinterface.h>
#include <fcitx/userinterfacemanager.h>
#include <fcntl.h>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define FCITX_UNIKEY_DEBUG() FCITX_LOGC(::fcitx::unikey, Debug)

namespace fcitx {

namespace {

FCITX_DEFINE_LOG_CATEGORY(unikey, "unikey");

constexpr auto CONVERT_BUF_SIZE = 1024;
constexpr auto MAX_LENGTH_VNWORD = 7;
const unsigned int Unikey_OC[] = {CONV_CHARSET_XUTF8,  CONV_CHARSET_TCVN3,
                                  CONV_CHARSET_VNIWIN, CONV_CHARSET_VIQR,
                                  CONV_CHARSET_BKHCM2, CONV_CHARSET_UNI_CSTRING,
                                  CONV_CHARSET_UNIREF, CONV_CHARSET_UNIREF_HEX};
constexpr unsigned int NUM_OUTPUTCHARSET = FCITX_ARRAY_SIZE(Unikey_OC);
static_assert(NUM_OUTPUTCHARSET == UkConvI18NAnnotation::enumLength);

bool isWordBreakSym(unsigned char c) { return WordBreakSyms.contains(c); }

bool isWordAutoCommit(unsigned char c) {
    static const std::unordered_set<unsigned char> WordAutoCommit = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'b', 'c',
        'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n', 'p', 'q', 'r', 's',
        't', 'v', 'x', 'z', 'B', 'C', 'F', 'G', 'H', 'J', 'K', 'L',
        'M', 'N', 'P', 'Q', 'R', 'S', 'T', 'V', 'X', 'Z'};
    return WordAutoCommit.contains(c);
}

VnLexiName charToVnLexi(uint32_t ch) {
    static const std::unordered_map<uint32_t, VnLexiName> map = []() {
        std::unordered_map<uint32_t, VnLexiName> result;
        for (int i = 0; i < vnl_lastChar; i++) {
            result.insert({UnicodeTable[i], static_cast<VnLexiName>(i)});
        }
        return result;
    }();

    if (auto search = map.find(ch); search != map.end()) {
        return search->second;
    }
    return vnl_nonVnChar;
}

bool isVnChar(uint32_t ch) { return charToVnLexi(ch) != vnl_nonVnChar; }

// code from x-unikey, for convert charset that not is XUtf-8
int latinToUtf(unsigned char *dst, const unsigned char *src, int inSize,
               int *pOutSize) {
    int i;
    int outLeft;
    unsigned char ch;

    outLeft = *pOutSize;

    for (i = 0; i < inSize; i++) {
        ch = *src++;
        if (ch < 0x80) {
            outLeft -= 1;
            if (outLeft >= 0) {
                *dst++ = ch;
            }
        } else {
            outLeft -= 2;
            if (outLeft >= 0) {
                *dst++ = (0xC0 | ch >> 6);
                *dst++ = (0x80 | (ch & 0x3F));
            }
        }
    }

    *pOutSize = outLeft;
    return (outLeft >= 0);
}

} // namespace

class UnikeyState final : public InputContextProperty {
public:
    UnikeyState(UnikeyEngine *engine, InputContext *ic)
        : engine_(engine), uic_(engine->im()), ic_(ic) {}

    void keyEvent(KeyEvent &keyEvent) {
        // Ignore all key release.
        if (keyEvent.isRelease()) {
            if (keyEvent.rawKey().check(FcitxKey_Shift_L) ||
                keyEvent.rawKey().check(FcitxKey_Shift_R)) {
                lastShiftPressed_ = FcitxKey_None;
            }
            return;
        }

        if (keyEvent.key().isSimple() &&
            !keyEvent.rawKey().check(FcitxKey_space)) {
            rebuildPreedit();
        }
        preedit(keyEvent);

        // check last keyevent with shift
        if (keyEvent.rawKey().sym() >= FcitxKey_space &&
            keyEvent.rawKey().sym() <= FcitxKey_asciitilde) {
            lastKeyWithShift_ =
                keyEvent.rawKey().states().test(KeyState::Shift);
        } else {
            lastKeyWithShift_ = false;
        } // end check last keyevent with shift
    }
    void preedit(KeyEvent &keyEvent);

    // A function for handling the key that is ignored by unikey and should
    // commit the buffer.
    void handleIgnoredKey();
    void commit();
    void syncState(KeySym sym = FcitxKey_None);
    void updatePreedit();

    void eraseChars(int num_chars) {
        int i;
        int k;
        unsigned char c;
        k = num_chars;

        for (i = preeditStr_.length() - 1; i >= 0 && k > 0; i--) {
            c = preeditStr_.at(i);

            // count down if byte is begin byte of utf-8 char
            if (c < (unsigned char)'\x80' || c >= (unsigned char)'\xC0') {
                k--;
            }
        }

        preeditStr_.erase(i + 1);
    }

    void reset() {
        uic_.resetBuf();
        preeditStr_.clear();
        updatePreedit();
        lastShiftPressed_ = FcitxKey_None;
    }

    void rebuildFromSurroundingText() {
        if (mayRebuildStateFromSurroundingText_) {
            mayRebuildStateFromSurroundingText_ = false;
        } else {
            return;
        }

        // Check if output charset is utf8, otherwise it doesn't make much
        // sense.
        // conflict with the rebuildPreedit feature
        if (!*engine_->config().surroundingText ||
            *engine_->config().modifySurroundingText ||
            *engine_->config().oc != UkConv::XUTF8) {
            return;
        }

        if (!uic_.isAtWordBeginning()) {
            return;
        }

        if (!ic_->capabilityFlags().test(CapabilityFlag::SurroundingText) ||
            !ic_->surroundingText().isValid()) {
            return;
        }
        // We need the character before the cursor.
        const auto &text = ic_->surroundingText().text();
        auto cursor = ic_->surroundingText().cursor();
        auto length = utf8::lengthValidated(text);
        if (length == utf8::INVALID_LENGTH) {
            return;
        }
        if (cursor <= 0 && cursor > length) {
            return;
        }

        uint32_t lastCharBeforeCursor;
        auto start = utf8::nextNChar(text.begin(), cursor - 1);
        auto end = utf8::getNextChar(start, text.end(), &lastCharBeforeCursor);
        if (lastCharBeforeCursor == utf8::INVALID_CHAR ||
            lastCharBeforeCursor == utf8::NOT_ENOUGH_SPACE) {
            return;
        }

        const auto isValidStateCharacter = [](char c) {
            return isWordAutoCommit(c) && !charutils::isdigit(c);
        };

        if (std::distance(start, end) != 1 ||
            !isValidStateCharacter(lastCharBeforeCursor)) {
            return;
        }

        // Reverse search for word auto commit.
        // all char for isWordAutoCommit == true would be ascii.
        while (start != text.begin() && isValidStateCharacter(*start) &&
               std::distance(start, end) < MAX_LENGTH_VNWORD) {
            --start;
        }

        // The loop will move the character on to an invalid character, if it
        // doesn't by pass the start point. Need to add by one to move it to the
        // starting point we expect.
        if (!isValidStateCharacter(*start)) {
            ++start;
        }

        assert(isValidStateCharacter(*start) && start >= text.begin());

        // Check if surrounding is not in a bigger part of word.
        if (start != text.begin()) {
            auto chr = utf8::getLastChar(text.begin(), start);
            if (isVnChar(chr)) {
                return;
            }
        }

        FCITX_UNIKEY_DEBUG()
            << "Rebuild surrounding with: \""
            << std::string_view(&*start, std::distance(start, end)) << "\"";
        for (; start != end; ++start) {
            uic_.putChar(*start);
            autoCommit_ = true;
        }
    }

    // After rebuild preedit, make sure nothing else calls commit
    void rebuildPreedit() {
        if (!*engine_->config().modifySurroundingText ||
            *engine_->config().oc != UkConv::XUTF8) {
            return;
        }

        if (!uic_.isAtWordBeginning()) {
            return;
        }

        if (!ic_->capabilityFlags().test(CapabilityFlag::SurroundingText) ||
            !ic_->surroundingText().isValid() ||
            !ic_->surroundingText().selectedText().empty()) {
            return;
        }

        const auto &text = ic_->surroundingText().text();
        auto cursor = ic_->surroundingText().cursor();
        auto length = utf8::lengthValidated(text);
        if (length == utf8::INVALID_LENGTH) {
            return;
        }
        if (cursor <= 0 && cursor > length) {
            return;
        }

        // get the last word before the cursor
        std::vector<VnLexiName> chars;
        chars.reserve(MAX_LENGTH_VNWORD + 1);
        // We will check at most MAX_LENGTH_VNWORD + 1 character before curosr.
        // This ensures that the word is not longer than MAX_LENGTH_VNWORD.
        size_t startCharacter = 0;
        if (cursor >= MAX_LENGTH_VNWORD + 1) {
            startCharacter = cursor - MAX_LENGTH_VNWORD - 1;
        }
        auto start = utf8::nextNChar(text.begin(), startCharacter);
        // Get the string end at cursor.
        auto end = utf8::nextNChar(start, cursor - startCharacter);
        // Scan from start to cursor, if hit a non Vn char, clear the buffer,
        // otherwise append to it.
        for (uint32_t unicode :
             utf8::MakeUTF8CharRange(std::string_view(&*start, end - start))) {
            auto ch = charToVnLexi(unicode);
            if (ch == vnl_nonVnChar) {
                chars.clear();
            } else {
                chars.push_back(ch);
            }
        }

        length = chars.size();
        if (length <= 0 || length > MAX_LENGTH_VNWORD) {
            return;
        }

        for (auto ch : chars) {
            uic_.rebuildChar(ch);
            syncState();
        }

        ic_->deleteSurroundingText(-length, length);
        updatePreedit();
    }

    bool mayRebuildStateFromSurroundingText_ = false;

private:
    UnikeyEngine *engine_;
    UnikeyInputContext uic_;
    InputContext *ic_;
    bool lastKeyWithShift_ = false;
    std::string preeditStr_;
    bool autoCommit_ = false;
    KeySym lastShiftPressed_ = FcitxKey_None;
};

UnikeyEngine::UnikeyEngine(Instance *instance)
    : instance_(instance), factory_([this](InputContext &ic) {
          return new UnikeyState(this, &ic);
      }) {
    instance_->inputContextManager().registerProperty("unikey-state",
                                                      &factory_);

    auto &uiManager = instance_->userInterfaceManager();
    inputMethodAction_ = std::make_unique<SimpleAction>();
    inputMethodAction_->setIcon("document-edit");
    inputMethodAction_->setShortText(_("Input Method"));
    uiManager.registerAction("unikey-input-method", inputMethodAction_.get());

    inputMethodMenu_ = std::make_unique<Menu>();
    inputMethodAction_->setMenu(inputMethodMenu_.get());

    for (UkInputMethod im : {UkTelex, UkVni, UkViqr, UkMsVi, UkUsrIM,
                             UkSimpleTelex, UkSimpleTelex2}) {
        inputMethodSubAction_.emplace_back(std::make_unique<SimpleAction>());
        auto *action = inputMethodSubAction_.back().get();
        action->setShortText(UkInputMethodI18NAnnotation::toString(im));
        action->setCheckable(true);
        uiManager.registerAction(
            "unikey-input-method-" + UkInputMethodToString(im), action);
        connections_.emplace_back(action->connect<SimpleAction::Activated>(
            [this, im](InputContext *ic) {
                config_.im.setValue(im);
                populateConfig();
                safeSaveAsIni(config_, "conf/unikey.conf");
                updateInputMethodAction(ic);
            }));

        inputMethodMenu_->addAction(action);
    }

    charsetAction_ = std::make_unique<SimpleAction>();
    charsetAction_->setShortText(_("Output charset"));
    charsetAction_->setIcon("character-set");
    uiManager.registerAction("unikey-charset", charsetAction_.get());
    charsetMenu_ = std::make_unique<Menu>();
    charsetAction_->setMenu(charsetMenu_.get());

    for (UkConv conv : {UkConv::XUTF8, UkConv::TCVN3, UkConv::VNIWIN,
                        UkConv::VIQR, UkConv::BKHCM2, UkConv::UNI_CSTRING,
                        UkConv::UNIREF, UkConv::UNIREF_HEX}) {
        charsetSubAction_.emplace_back(std::make_unique<SimpleAction>());
        auto *action = charsetSubAction_.back().get();
        action->setShortText(UkConvI18NAnnotation::toString(conv));
        action->setCheckable(true);
        connections_.emplace_back(action->connect<SimpleAction::Activated>(
            [this, conv](InputContext *ic) {
                config_.oc.setValue(conv);
                populateConfig();
                safeSaveAsIni(config_, "conf/unikey.conf");
                updateCharsetAction(ic);
            }));
        uiManager.registerAction("unikey-charset-" + UkConvToString(conv),
                                 action);
        charsetMenu_->addAction(action);
    }
    spellCheckAction_ = std::make_unique<SimpleAction>();
    spellCheckAction_->setLongText(_("Spell check"));
    spellCheckAction_->setIcon("tools-check-spelling");
    connections_.emplace_back(
        spellCheckAction_->connect<SimpleAction::Activated>(
            [this](InputContext *ic) {
                config_.spellCheck.setValue(!*config_.spellCheck);
                populateConfig();
                safeSaveAsIni(config_, "conf/unikey.conf");
                updateSpellAction(ic);
            }));
    uiManager.registerAction("unikey-spell-check", spellCheckAction_.get());
    macroAction_ = std::make_unique<SimpleAction>();
    macroAction_->setLongText(_("Macro"));
    macroAction_->setIcon("edit-find");
    connections_.emplace_back(macroAction_->connect<SimpleAction::Activated>(
        [this](InputContext *ic) {
            config_.macro.setValue(!*config_.macro);
            populateConfig();
            safeSaveAsIni(config_, "conf/unikey.conf");
            updateMacroAction(ic);
        }));
    uiManager.registerAction("unikey-macro", macroAction_.get());

    eventWatchers_.emplace_back(instance_->watchEvent(
        EventType::InputContextSurroundingTextUpdated,
        EventWatcherPhase::PostInputMethod, [this](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            auto *ic = icEvent.inputContext();
            auto *state = ic->propertyFor(&factory_);
            state->mayRebuildStateFromSurroundingText_ = true;
        }));

    reloadConfig();
}

UnikeyEngine::~UnikeyEngine() {}

void UnikeyEngine::activate(const InputMethodEntry & /*entry*/,
                            InputContextEvent &event) {
    auto &statusArea = event.inputContext()->statusArea();
    statusArea.addAction(StatusGroup::InputMethod, inputMethodAction_.get());
    statusArea.addAction(StatusGroup::InputMethod, charsetAction_.get());
    statusArea.addAction(StatusGroup::InputMethod, spellCheckAction_.get());
    statusArea.addAction(StatusGroup::InputMethod, macroAction_.get());

    auto *ic = event.inputContext();
    updateUI(ic);
    auto *state = ic->propertyFor(&factory_);
    if (ic->capabilityFlags().test(CapabilityFlag::SurroundingText)) {
        state->mayRebuildStateFromSurroundingText_ = true;
    }
}

void UnikeyEngine::deactivate(const InputMethodEntry &entry,
                              InputContextEvent &event) {
    if (event.type() == EventType::InputContextSwitchInputMethod) {
        auto *state = event.inputContext()->propertyFor(&factory_);
        state->commit();
    }
    reset(entry, event);
}

void UnikeyEngine::keyEvent(const InputMethodEntry & /*entry*/,
                            KeyEvent &keyEvent) {
    auto *ic = keyEvent.inputContext();
    auto *state = ic->propertyFor(&factory_);
    state->rebuildFromSurroundingText();
    state->keyEvent(keyEvent);
}

void UnikeyState::preedit(KeyEvent &keyEvent) {
    auto sym = keyEvent.rawKey().sym();
    auto state = keyEvent.rawKey().states();

    // We try to detect Press and release of two different shift.
    // The sequence we want to detect is:
    if (keyEvent.rawKey().check(FcitxKey_Shift_L) ||
        keyEvent.rawKey().check(FcitxKey_Shift_R)) {
        if (lastShiftPressed_ == FcitxKey_None) {
            lastShiftPressed_ = keyEvent.rawKey().sym();
        } else if (lastShiftPressed_ != keyEvent.rawKey().sym()) {
            // Another shift is pressed, do restore Key.
            uic_.restoreKeyStrokes();
            syncState(keyEvent.rawKey().sym());
            updatePreedit();
            lastShiftPressed_ = FcitxKey_None;
            keyEvent.filterAndAccept();
            return;
        }
    } else {
        // We pressed something else, reset the state.
        lastShiftPressed_ = FcitxKey_None;
    }

    if (state.testAny(KeyState::Ctrl_Alt) || sym == FcitxKey_Control_L ||
        sym == FcitxKey_Control_R || sym == FcitxKey_Tab ||
        sym == FcitxKey_Return || sym == FcitxKey_Delete ||
        sym == FcitxKey_KP_Enter ||
        (sym >= FcitxKey_Home && sym <= FcitxKey_Insert) ||
        (sym >= FcitxKey_KP_Home && sym <= FcitxKey_KP_Delete)) {
        handleIgnoredKey();
        return;
    }
    if (state.test(KeyState::Super)) {
        return;
    }
    if ((sym >= FcitxKey_Caps_Lock && sym <= FcitxKey_Hyper_R) ||
        sym == FcitxKey_Shift_L || sym == FcitxKey_Shift_R) {
        return;
    }
    if (sym == FcitxKey_BackSpace) {
        // capture BackSpace
        uic_.backspacePress();

        if (uic_.backspaces() == 0 || preeditStr_.empty()) {
            commit();
            return;
        }
        if (static_cast<int>(preeditStr_.length()) <= uic_.backspaces()) {
            preeditStr_.clear();
            autoCommit_ = true;
        } else {
            eraseChars(uic_.backspaces());
        }

        // change tone position after press backspace
        if (uic_.bufChars() > 0) {
            if (engine_->config().oc.value() == UkConv::XUTF8) {
                preeditStr_.append(reinterpret_cast<const char *>(uic_.buf()),
                                   uic_.bufChars());
            } else {
                unsigned char buf[CONVERT_BUF_SIZE];
                int bufSize = CONVERT_BUF_SIZE;

                latinToUtf(buf, uic_.buf(), uic_.bufChars(), &bufSize);
                preeditStr_.append((const char *)buf,
                                   CONVERT_BUF_SIZE - bufSize);
            }

            autoCommit_ = false;
        }
        updatePreedit();

        keyEvent.filterAndAccept();
        return;
    }
    if (sym >= FcitxKey_KP_Multiply && sym <= FcitxKey_KP_9) {
        handleIgnoredKey();
        return;
    }
    if (sym >= FcitxKey_space && sym <= FcitxKey_asciitilde) {
        // capture ascii printable char
        uic_.setCapsState(state.test(KeyState::Shift),
                          state.test(KeyState::CapsLock));

        // process sym

        // auto commit word that never need to change later in preedit string
        // (like consonant - phu am) if macro enabled, then not auto commit.
        // Because macro may change any word
        if (!*engine_->config().macro &&
            (uic_.isAtWordBeginning() || autoCommit_) &&
            // conflict with the rebuildPreedit feature
            !*engine_->config().modifySurroundingText) {
            if (isWordAutoCommit(sym)) {
                uic_.putChar(sym);
                autoCommit_ = true;
                return;
            }
        } // end auto commit

        if ((*engine_->config().im == UkTelex ||
             *engine_->config().im == UkSimpleTelex2) &&
            !*engine_->config().process_w_at_begin &&
            uic_.isAtWordBeginning() &&
            (sym == FcitxKey_w || sym == FcitxKey_W)) {
            uic_.putChar(sym);
            if (!*engine_->config().macro) {
                return;
            }
            preeditStr_.append(sym == FcitxKey_w ? "w" : "W");
            updatePreedit();
            keyEvent.filterAndAccept();
            return;
        }

        autoCommit_ = false;

        // shift + space, shift + shift event
        if (!lastKeyWithShift_ && state.test(KeyState::Shift) &&
            sym == FcitxKey_space && !uic_.isAtWordBeginning()) {
            uic_.restoreKeyStrokes();
        } else {
            uic_.filter(sym);
        }
        // end shift + space
        // end process sym

        syncState(sym);

        // commit string: if need
        if (!preeditStr_.empty()) {
            if (preeditStr_.back() == sym && isWordBreakSym(sym)) {
                commit();
                keyEvent.filterAndAccept();
                return;
            }
        }
        // end commit string

        updatePreedit();
        keyEvent.filterAndAccept();
        return;
    } // end capture printable char

    // non process key
    handleIgnoredKey();
}

void UnikeyEngine::reset(const InputMethodEntry & /*entry*/,
                         InputContextEvent &event) {
    auto *state = event.inputContext()->propertyFor(&factory_);
    state->reset();
    if (event.type() == EventType::InputContextReset) {
        if (event.inputContext()->capabilityFlags().test(
                CapabilityFlag::SurroundingText)) {
            state->mayRebuildStateFromSurroundingText_ = true;
        }
    }
}

void UnikeyEngine::populateConfig() {
    UnikeyOptions ukopt;
    memset(&ukopt, 0, sizeof(ukopt));
    ukopt.macroEnabled = *config_.macro;
    ukopt.spellCheckEnabled = *config_.spellCheck;
    ukopt.autoNonVnRestore = *config_.autoNonVnRestore;
    ukopt.modernStyle = *config_.modernStyle;
    ukopt.freeMarking = *config_.freeMarking;
    im_.setInputMethod(*config_.im);
    im_.setOutputCharset(Unikey_OC[static_cast<int>(*config_.oc)]);
    im_.setOptions(&ukopt);
}

void UnikeyEngine::reloadConfig() {
    readAsIni(config_, "conf/unikey.conf");
    reloadKeymap();
    populateConfig();
    reloadMacroTable();
}

void UnikeyEngine::reloadKeymap() {
    // Keymap need to be reloaded before populateConfig.
    auto keymapFile = StandardPaths::global().open(StandardPathsType::PkgConfig,
                                                   "unikey/keymap.txt");
    if (keymapFile.isValid()) {
        UkLoadKeyMap(keymapFile.fd(), im_.sharedMem()->usrKeyMap);
        im_.sharedMem()->usrKeyMapLoaded = true;
    } else {
        im_.sharedMem()->usrKeyMapLoaded = false;
    }
}

void UnikeyEngine::save() {}

std::string UnikeyEngine::subMode(const InputMethodEntry & /*entry*/,
                                  InputContext & /*inputContext*/) {
    return UkInputMethodI18NAnnotation::toString(*config_.im);
}
void UnikeyEngine::updateMacroAction(InputContext *ic) {
    macroAction_->setChecked(*config_.macro);
    macroAction_->setShortText(*config_.macro ? _("Macro Enabled")
                                              : _("Macro Disabled"));
    macroAction_->update(ic);
}

void UnikeyEngine::updateSpellAction(InputContext *ic) {
    spellCheckAction_->setChecked(*config_.spellCheck);
    spellCheckAction_->setShortText(*config_.spellCheck
                                        ? _("Spell Check Enabled")
                                        : _("Spell Check Disabled"));
    spellCheckAction_->update(ic);
}

void UnikeyEngine::updateInputMethodAction(InputContext *ic) {
    for (size_t i = 0; i < inputMethodSubAction_.size(); i++) {
        inputMethodSubAction_[i]->setChecked(i ==
                                             static_cast<size_t>(*config_.im));
        inputMethodSubAction_[i]->update(ic);
    }
    inputMethodAction_->setLongText(
        UkInputMethodI18NAnnotation::toString(*config_.im));
    inputMethodAction_->update(ic);
}

void UnikeyEngine::updateCharsetAction(InputContext *ic) {
    for (size_t i = 0; i < charsetSubAction_.size(); i++) {
        charsetSubAction_[i]->setChecked(i == static_cast<size_t>(*config_.oc));
        charsetSubAction_[i]->update(ic);
    }
    charsetAction_->setLongText(UkConvI18NAnnotation::toString(*config_.oc));
    charsetAction_->update(ic);
}

void UnikeyEngine::updateUI(InputContext *ic) {
    updateInputMethodAction(ic);
    updateCharsetAction(ic);
    updateMacroAction(ic);
    updateSpellAction(ic);
}

void UnikeyState::handleIgnoredKey() {
    uic_.filter(0);
    syncState();
    commit();
}

void UnikeyState::commit() {
    if (!preeditStr_.empty()) {
        ic_->commitString(preeditStr_);
    }
    reset();
}

void UnikeyState::syncState(KeySym sym) {
    // process result of ukengine
    if (uic_.backspaces() > 0) {
        if (static_cast<int>(preeditStr_.length()) <= uic_.backspaces()) {
            preeditStr_.clear();
        } else {
            eraseChars(uic_.backspaces());
        }
    }

    if (uic_.bufChars() > 0) {
        if (*engine_->config().oc == UkConv::XUTF8) {
            preeditStr_.append(reinterpret_cast<const char *>(uic_.buf()),
                               uic_.bufChars());
        } else {
            unsigned char buf[CONVERT_BUF_SIZE + 1];
            int bufSize = CONVERT_BUF_SIZE;

            latinToUtf(buf, uic_.buf(), uic_.bufChars(), &bufSize);
            preeditStr_.append((const char *)buf, CONVERT_BUF_SIZE - bufSize);
        }
    } else if (sym != FcitxKey_Shift_L && sym != FcitxKey_Shift_R &&
               sym != FcitxKey_None) // if ukengine not process
    {
        preeditStr_.append(utf8::UCS4ToUTF8(sym));
    }
    // end process result of ukengine
}

void UnikeyState::updatePreedit() {
    auto &inputPanel = ic_->inputPanel();

    inputPanel.reset();

    if (!preeditStr_.empty()) {
        const auto useClientPreedit =
            ic_->capabilityFlags().test(CapabilityFlag::Preedit);
        Text preedit(preeditStr_,
                     useClientPreedit && *engine_->config().displayUnderline
                         ? TextFormatFlag::Underline
                         : TextFormatFlag::NoFlag);
        preedit.setCursor(preeditStr_.size());
        if (useClientPreedit) {
            inputPanel.setClientPreedit(preedit);
        } else {
            inputPanel.setPreedit(preedit);
        }
    }
    ic_->updatePreedit();
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

} // namespace fcitx

FCITX_ADDON_FACTORY_V2(unikey, fcitx::UnikeyFactory)
