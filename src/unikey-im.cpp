/*
 * SPDX-FileCopyrightText: 2012-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "unikey-im.h"
#include <fcitx-utils/charutils.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/menu.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterfacemanager.h>
#include <vnconv.h>

namespace fcitx {

FCITX_DEFINE_LOG_CATEGORY(unikey, "unikey");
#define FCITX_UNIKEY_DEBUG() FCITX_LOGC(unikey, Debug)

namespace {

constexpr auto CONVERT_BUF_SIZE = 1024;
constexpr auto MAX_CONTEXT_SIZE = 20;
static const unsigned int Unikey_OC[] = {
    CONV_CHARSET_XUTF8,  CONV_CHARSET_TCVN3,     CONV_CHARSET_VNIWIN,
    CONV_CHARSET_VIQR,   CONV_CHARSET_BKHCM2,    CONV_CHARSET_UNI_CSTRING,
    CONV_CHARSET_UNIREF, CONV_CHARSET_UNIREF_HEX};
static constexpr unsigned int NUM_OUTPUTCHARSET = FCITX_ARRAY_SIZE(Unikey_OC);
static_assert(NUM_OUTPUTCHARSET == UkConvI18NAnnotation::enumLength);

static const unsigned char WordBreakSyms[] = {
    ',', ';', ':', '.', '\"', '\'', '!', '?', ' ', '<', '>',
    '=', '+', '-', '*', '/',  '\\', '_', '~', '`', '@', '#',
    '$', '%', '^', '&', '(',  ')',  '{', '}', '[', ']', '|'};

static bool isWordAutoCommit(unsigned char c) {
    static const std::unordered_set<unsigned char> WordAutoCommit = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'b', 'c',
        'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n', 'p', 'q', 'r', 's',
        't', 'v', 'x', 'z', 'B', 'C', 'F', 'G', 'H', 'J', 'K', 'L',
        'M', 'N', 'P', 'Q', 'R', 'S', 'T', 'V', 'X', 'Z'};
    return WordAutoCommit.count(c);
}

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
            if (outLeft >= 0)
                *dst++ = ch;
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
        int i, k;
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

        if (!*engine_->config().surroundingText) {
            return;
        }

        if (!uic_.isAtWordBeginning()) {
            return;
        }

        if (!ic_->capabilityFlags().test(CapabilityFlag::SurroundingText) ||
            !ic_->surroundingText().isValid()) {
            return;
        }
        // We need text before the cursor.
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
        if (std::distance(start, end) != 1 ||
            !isWordAutoCommit(lastCharBeforeCursor) ||
            charutils::isdigit(lastCharBeforeCursor)) {
            return;
        }

        // Reverse search for word auto commit.
        // all char for isWordAutoCommit == true would be ascii.
        while (start != text.begin() && isWordAutoCommit(*start) &&
               !charutils::isdigit(lastCharBeforeCursor) &&
               std::distance(start, end) < MAX_CONTEXT_SIZE) {
            --start;
        }
        FCITX_UNIKEY_DEBUG()
            << "Rebuild surrounding with: "
            << std::string_view(&*start, std::distance(start, end));
        for (; start != end; ++start) {
            uic_.putChar(*start);
            autoCommit_ = true;
        }
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
        auto action = inputMethodSubAction_.back().get();
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
        auto action = charsetSubAction_.back().get();
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

void UnikeyEngine::activate(const InputMethodEntry &,
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
    reset(entry, event);
}

void UnikeyEngine::keyEvent(const InputMethodEntry &, KeyEvent &keyEvent) {
    auto ic = keyEvent.inputContext();
    auto state = ic->propertyFor(&factory_);
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
            return keyEvent.filterAndAccept();
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
    } else if (state.test(KeyState::Super)) {
        return;
    } else if ((sym >= FcitxKey_Caps_Lock && sym <= FcitxKey_Hyper_R) ||
               sym == FcitxKey_Shift_L || sym == FcitxKey_Shift_R) {
        return;
    } else if (sym == FcitxKey_BackSpace) {
        // capture BackSpace
        uic_.backspacePress();

        if (uic_.backspaces() == 0 || preeditStr_.empty()) {
            commit();
            return;
        } else {
            if (static_cast<int>(preeditStr_.length()) <= uic_.backspaces()) {
                preeditStr_.clear();
                autoCommit_ = true;
            } else {
                eraseChars(uic_.backspaces());
            }

            // change tone position after press backspace
            if (uic_.bufChars() > 0) {
                if (engine_->config().oc.value() == UkConv::XUTF8) {
                    preeditStr_.append(
                        reinterpret_cast<const char *>(uic_.buf()),
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
        }
        return keyEvent.filterAndAccept();
    } else if (sym >= FcitxKey_KP_Multiply && sym <= FcitxKey_KP_9) {
        handleIgnoredKey();
        return;
    } else if (sym >= FcitxKey_space && sym <= FcitxKey_asciitilde) {
        // capture ascii printable char
        uic_.setCapsState(state.test(KeyState::Shift),
                          state.test(KeyState::CapsLock));

        // process sym

        // auto commit word that never need to change later in preedit string
        // (like consonant - phu am) if macro enabled, then not auto commit.
        // Because macro may change any word
        if (!*engine_->config().macro &&
            (uic_.isAtWordBeginning() || autoCommit_)) {
            if (isWordAutoCommit(sym)) {
                uic_.putChar(sym);
                autoCommit_ = true;
                return;
            }
        } // end auto commit

        if ((*engine_->config().im == UkTelex ||
             *engine_->config().im == UkSimpleTelex2) &&
            *engine_->config().process_w_at_begin == false &&
            uic_.isAtWordBeginning() &&
            (sym == FcitxKey_w || sym == FcitxKey_W)) {
            uic_.putChar(sym);
            if (!*engine_->config().macro) {
                return;
            } else {
                preeditStr_.append(sym == FcitxKey_w ? "w" : "W");
                updatePreedit();
                return keyEvent.filterAndAccept();
            }
        }

        autoCommit_ = false;

        // shift + space, shift + shift event
        if (lastKeyWithShift_ == false && state.test(KeyState::Shift) &&
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
            for (auto wordBreakSym : WordBreakSyms) {
                if (wordBreakSym == preeditStr_.back() && wordBreakSym == sym) {
                    commit();
                    return keyEvent.filterAndAccept();
                }
            }
        }
        // end commit string

        updatePreedit();
        return keyEvent.filterAndAccept();
    } // end capture printable char

    // non process key
    handleIgnoredKey();
}

void UnikeyEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto state = event.inputContext()->propertyFor(&factory_);
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
    populateConfig();
    reloadMacroTable();
}

void UnikeyEngine::save() {}

std::string UnikeyEngine::subMode(const InputMethodEntry &, InputContext &) {
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
        Text preedit(preeditStr_, useClientPreedit ? TextFormatFlag::Underline
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

FCITX_ADDON_FACTORY(fcitx::UnikeyFactory)
