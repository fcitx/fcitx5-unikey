/*
 * SPDX-FileCopyrightText: 2012-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "unikey-im.h"
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/menu.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterfacemanager.h>
#include <vnconv.h>

constexpr auto CONVERT_BUF_SIZE = 1024;
static const unsigned int Unikey_OC[] = {
    CONV_CHARSET_XUTF8,  CONV_CHARSET_TCVN3,     CONV_CHARSET_VNIWIN,
    CONV_CHARSET_VIQR,   CONV_CHARSET_BKHCM2,    CONV_CHARSET_UNI_CSTRING,
    CONV_CHARSET_UNIREF, CONV_CHARSET_UNIREF_HEX};
static const unsigned int NUM_OUTPUTCHARSET = FCITX_ARRAY_SIZE(Unikey_OC);

static const unsigned char WordBreakSyms[] = {
    ',', ';', ':', '.', '\"', '\'', '!', '?', ' ', '<', '>',
    '=', '+', '-', '*', '/',  '\\', '_', '~', '`', '@', '#',
    '$', '%', '^', '&', '(',  ')',  '{', '}', '[', ']', '|'};

static const unsigned char WordAutoCommit[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'b', 'c',
    'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n', 'p', 'q', 'r', 's',
    't', 'v', 'x', 'z', 'B', 'C', 'F', 'G', 'H', 'J', 'K', 'L',
    'M', 'N', 'P', 'Q', 'R', 'S', 'T', 'V', 'X', 'Z'};

namespace fcitx {

namespace {

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

    void keyEvent(fcitx::KeyEvent &keyEvent) {
        // Ignore all key release.
        if (keyEvent.isRelease()) {
            return;
        }
        preedit(keyEvent);

        // check last keyevent with shift
        if (keyEvent.origKey().sym() >= FcitxKey_space &&
            keyEvent.origKey().sym() <= FcitxKey_asciitilde) {
            lastKeyWithShift_ =
                keyEvent.origKey().states().test(KeyState::Shift);
        } else {
            lastKeyWithShift_ = false;
        } // end check last keyevent with shift
    }
    void preedit(fcitx::KeyEvent &keyEvent);
    void commit();
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
    }

private:
    UnikeyEngine *engine_;
    UnikeyInputContext uic_;
    InputContext *ic_;
    bool lastKeyWithShift_ = false;
    std::string preeditStr_;
    bool autoCommit_ = false;
};

fcitx::UnikeyEngine::UnikeyEngine(fcitx::Instance *instance)
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

    reloadConfig();
}

fcitx::UnikeyEngine::~UnikeyEngine() {}

} // namespace fcitx

void fcitx::UnikeyEngine::activate(const fcitx::InputMethodEntry &,
                                   fcitx::InputContextEvent &event) {
    auto &statusArea = event.inputContext()->statusArea();
    statusArea.addAction(StatusGroup::InputMethod, inputMethodAction_.get());
    statusArea.addAction(StatusGroup::InputMethod, charsetAction_.get());
    statusArea.addAction(StatusGroup::InputMethod, spellCheckAction_.get());
    statusArea.addAction(StatusGroup::InputMethod, macroAction_.get());

    updateUI(event.inputContext());
}

void fcitx::UnikeyEngine::deactivate(const fcitx::InputMethodEntry &entry,
                                     fcitx::InputContextEvent &event) {
    auto &statusArea = event.inputContext()->statusArea();
    statusArea.clearGroup(StatusGroup::InputMethod);
    reset(entry, event);
}

void fcitx::UnikeyEngine::keyEvent(const fcitx::InputMethodEntry &,
                                   fcitx::KeyEvent &keyEvent) {
    auto ic = keyEvent.inputContext();
    auto state = ic->propertyFor(&factory_);
    state->keyEvent(keyEvent);
}

void fcitx::UnikeyState::preedit(fcitx::KeyEvent &keyEvent) {

    auto sym = keyEvent.rawKey().sym();
    auto state = keyEvent.rawKey().states();

    if (state.testAny(KeyState::Ctrl_Alt) || sym == FcitxKey_Control_L ||
        sym == FcitxKey_Control_R || sym == FcitxKey_Tab ||
        sym == FcitxKey_Return || sym == FcitxKey_Delete ||
        sym == FcitxKey_KP_Enter ||
        (sym >= FcitxKey_Home && sym <= FcitxKey_Insert) ||
        (sym >= FcitxKey_KP_Home && sym <= FcitxKey_KP_Delete)) {
        commit();
        return;
    } else if (state.test(KeyState::Super)) {
        return;
    } else if ((sym >= FcitxKey_Caps_Lock && sym <= FcitxKey_Hyper_R) ||
               ((!state.test(KeyState::Shift)) &&
                (sym == FcitxKey_Shift_L ||
                 sym == FcitxKey_Shift_R)) // when press one shift key
    ) {
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
        commit();
        return;
    } else if ((sym >= FcitxKey_space && sym <= FcitxKey_asciitilde) ||
               sym == FcitxKey_Shift_L ||
               sym == FcitxKey_Shift_R) // sure this have FcitxKey_SHIFT_MASK
    {
        // capture ascii printable char
        unsigned int i = 0;

        uic_.setCapsState(state.test(KeyState::Shift),
                          state.test(KeyState::CapsLock));

        // process sym

        // auto commit word that never need to change later in preedit string
        // (like consonant - phu am) if macro enabled, then not auto commit.
        // Because macro may change any word
        if (!*engine_->config().macro &&
            (uic_.isAtWordBeginning() || autoCommit_)) {
            for (i = 0; i < sizeof(WordAutoCommit); i++) {
                if (sym == WordAutoCommit[i]) {
                    uic_.putChar(sym);
                    autoCommit_ = true;
                    return;
                }
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
        if ((lastKeyWithShift_ == false && state.test(KeyState::Shift) &&
             sym == FcitxKey_space && !uic_.isAtWordBeginning()) ||
            (sym == FcitxKey_Shift_L ||
             sym == FcitxKey_Shift_R) // (&& state & FcitxKey_SHIFT_MASK), sure
                                      // this have FcitxKey_SHIFT_MASK
        ) {
            uic_.restoreKeyStrokes();
        } else {
            uic_.filter(sym);
        }
        // end shift + space, shift + shift event
        // end process sym

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
                preeditStr_.append((const char *)buf,
                                   CONVERT_BUF_SIZE - bufSize);
            }
        } else if (sym != FcitxKey_Shift_L &&
                   sym != FcitxKey_Shift_R) // if ukengine not process
        {
            preeditStr_.append(utf8::UCS4ToUTF8(sym));
        }
        // end process result of ukengine

        // commit string: if need
        if (preeditStr_.length() > 0) {
            unsigned int i;
            for (i = 0; i < sizeof(WordBreakSyms); i++) {
                if (WordBreakSyms[i] ==
                        preeditStr_.at(preeditStr_.length() - 1) &&
                    WordBreakSyms[i] == sym) {
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

    commit();
}

void fcitx::UnikeyEngine::reset(const fcitx::InputMethodEntry &,
                                fcitx::InputContextEvent &event) {
    auto state = event.inputContext()->propertyFor(&factory_);
    state->reset();
}

void fcitx::UnikeyEngine::populateConfig() {
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

void fcitx::UnikeyEngine::reloadConfig() {
    readAsIni(config_, "conf/unikey.conf");
    populateConfig();
    auto path = StandardPath::global().locate(StandardPath::Type::Config,
                                              "unikey/macro");

    if (!path.empty()) {
        im_.loadMacroTable(path.data());
    }
}

void fcitx::UnikeyEngine::save() {}

std::string fcitx::UnikeyEngine::subMode(const fcitx::InputMethodEntry &,
                                         fcitx::InputContext &) {
    return UkInputMethodI18NAnnotation::toString(*config_.im);
}
void fcitx::UnikeyEngine::updateMacroAction(InputContext *ic) {
    macroAction_->setChecked(*config_.macro);
    macroAction_->setShortText(*config_.macro ? _("Macro Enabled")
                                              : _("Macro Disabled"));
    macroAction_->update(ic);
}

void fcitx::UnikeyEngine::updateSpellAction(InputContext *ic) {
    spellCheckAction_->setChecked(*config_.spellCheck);
    spellCheckAction_->setShortText(*config_.spellCheck
                                        ? _("Spell Check Enabled")
                                        : _("Spell Check Disabled"));
    spellCheckAction_->update(ic);
}

void fcitx::UnikeyEngine::updateInputMethodAction(InputContext *ic) {
    for (size_t i = 0; i < inputMethodSubAction_.size(); i++) {
        inputMethodSubAction_[i]->setChecked(i ==
                                             static_cast<size_t>(*config_.im));
        inputMethodSubAction_[i]->update(ic);
    }
    inputMethodAction_->setLongText(
        UkInputMethodI18NAnnotation::toString(*config_.im));
    inputMethodAction_->update(ic);
}

void fcitx::UnikeyEngine::updateCharsetAction(InputContext *ic) {
    for (size_t i = 0; i < charsetSubAction_.size(); i++) {
        charsetSubAction_[i]->setChecked(i == static_cast<size_t>(*config_.oc));
        charsetSubAction_[i]->update(ic);
    }
    charsetAction_->setLongText(UkConvI18NAnnotation::toString(*config_.oc));
    charsetAction_->update(ic);
}

void fcitx::UnikeyEngine::updateUI(InputContext *ic) {
    updateInputMethodAction(ic);
    updateCharsetAction(ic);
    updateMacroAction(ic);
    updateSpellAction(ic);
}

void fcitx::UnikeyState::commit() {
    if (!preeditStr_.empty()) {
        ic_->commitString(preeditStr_);
    }
    reset();
}

void fcitx::UnikeyState::updatePreedit() {
    auto &inputPanel = ic_->inputPanel();

    inputPanel.reset();

    Text preedit(preeditStr_, TextFormatFlag::Underline);

    if (preeditStr_.size()) {
        preedit.setCursor(preeditStr_.size());
    }
    if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
        inputPanel.setClientPreedit(preedit);
        ic_->updatePreedit();
    } else {
        inputPanel.setPreedit(preedit);
    }
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

FCITX_ADDON_FACTORY(fcitx::UnikeyFactory)
