/*
 * SPDX-FileCopyrightText: 2026 Owen
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "unikey-config.h"
#include "unikey-directcommit.h"
#include "unikey-im.h"
#include <algorithm>
#include <cstddef>
#include <fcitx-config/iniparser.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx/action.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/menu.h>
#include <fcitx/text.h>
#include <fcitx/userinterface.h>
#include <fcitx/userinterfacemanager.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace fcitx {

namespace {

class InputModeCandidate : public CandidateWord {
public:
    InputModeCandidate(DirectCommit *owner, int mode, std::string text)
        : CandidateWord(Text(std::move(text))), owner_(owner), mode_(mode) {}

    void select(InputContext * /*ic*/) const override {
        owner_->selectMode(mode_);
    }

private:
    DirectCommit *owner_;
    int mode_;
};

std::string lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

// Mode of the first entry matching program, or AppModeGlobal.
int findAppMode(const UnikeyConfig &config, const std::string &program) {
    const auto name = lower(program);
    for (const auto &entry : *config.appModes) {
        auto pos = entry.find('=');
        if (pos == 0 || pos == std::string::npos) {
            continue;
        }
        auto app = lower(stringutils::trim(entry.substr(0, pos)));
        auto value = lower(stringutils::trim(entry.substr(pos + 1)));
        if (app.empty() || name.find(app) == std::string::npos) {
            continue;
        }
        for (int i = 0; i <= AppModeOff; i++) {
            if (value == AppModeNames[i]) {
                return i;
            }
        }
    }
    return AppModeGlobal;
}

// Replace the entries matching program, AppModeGlobal only removes them.
void setAppMode(UnikeyConfig &config, const std::string &program, int mode) {
    const auto name = lower(program);
    auto appModes = *config.appModes;
    appModes.erase(std::remove_if(appModes.begin(), appModes.end(),
                                  [&name](const std::string &entry) {
                                      auto app = lower(stringutils::trim(
                                          entry.substr(0, entry.find('='))));
                                      return !app.empty() &&
                                             name.find(app) !=
                                                 std::string::npos;
                                  }),
                   appModes.end());
    if (mode != AppModeGlobal) {
        appModes.push_back(name + "=" + AppModeNames[mode]);
    }
    config.appModes.setValue(std::move(appModes));
}

// Order of the per app modes in the popup.
constexpr int AppModeOrder[] = {AppModeGlobal, 0, 1, 2, 3, 4, 5, AppModeOff};

std::string appModeLabel(int mode) {
    if (mode == AppModeGlobal) {
        return _("Use global setting");
    }
    return InputModeI18NAnnotation::toString(static_cast<InputMode>(mode));
}

} // namespace

bool DirectCommit::resolveMode(InputMode *mode) const {
    *mode = *engine_->config().inputMode;
    const int appMode = findAppMode(engine_->config(), ic_->program());
    if (appMode != AppModeGlobal) {
        *mode = static_cast<InputMode>(appMode);
    }
    return *mode != InputMode::Off;
}

InputMode DirectCommit::inputMode() const {
    InputMode mode;
    resolveMode(&mode);
    return mode;
}


void DirectCommit::openModeList() {
    const auto program = ic_->program();
    const int current = program.empty()
                            ? static_cast<int>(*engine_->config().inputMode)
                            : findAppMode(engine_->config(), program);
    auto list = std::make_unique<CommonCandidateList>();
    list->setPageSize(AppModeGlobal + 1);
    list->setLayoutHint(CandidateLayoutHint::Vertical);
    list->setCursorPositionAfterPaging(CursorPositionAfterPaging::ResetToFirst);
    int cursor = 0;
    for (int mode : AppModeOrder) {
        // Without a program name only the global mode can be changed.
        if (program.empty() && mode == AppModeGlobal) {
            continue;
        }
        std::string label = appModeLabel(mode);
        if (mode == current) {
            cursor = list->totalSize();
            label += " *";
        }
        list->append<InputModeCandidate>(this, mode, std::move(label));
    }
    list->setGlobalCursorIndex(cursor);
    auto &inputPanel = ic_->inputPanel();
    inputPanel.reset();
    inputPanel.setAuxUp(Text(program.empty()
                                 ? std::string(_("Input mode"))
                                 : std::string(_("Input mode for ")) + program));
    inputPanel.setCandidateList(std::move(list));
    modeListOpen_ = true;
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void DirectCommit::closeModeList() {
    modeListOpen_ = false;
    ic_->inputPanel().reset();
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void DirectCommit::handleModeListKey(KeyEvent &keyEvent) {
    keyEvent.filterAndAccept();
    if (keyEvent.isRelease()) {
        return;
    }
    auto list = ic_->inputPanel().candidateList();
    const auto &key = keyEvent.key();
    if (!list || key.check(FcitxKey_Escape)) {
        closeModeList();
        return;
    }
    if (int index = key.digitSelection(); index >= 0) {
        if (index < list->size()) {
            list->candidate(index).select(ic_);
        }
        return;
    }
    auto *movable = list->toCursorMovable();
    if (key.check(FcitxKey_Up)) {
        movable->prevCandidate();
    } else if (key.check(FcitxKey_Down)) {
        movable->nextCandidate();
    } else if (key.check(FcitxKey_Return) || key.check(FcitxKey_KP_Enter)) {
        if (list->cursorIndex() >= 0) {
            list->candidate(list->cursorIndex()).select(ic_);
        }
        return;
    }
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void DirectCommit::selectMode(int mode) {
    closeModeList();
    auto &config = engine_->config();
    const auto program = ic_->program();
    if (program.empty()) {
        // Unknown app, change the global mode instead.
        config.inputMode.setValue(static_cast<InputMode>(mode));
    } else {
        setAppMode(config, program, mode);
    }
    safeSaveAsIni(config, "conf/unikey.conf");
    engine_->updateInputModeAction(ic_);
}

void UnikeyEngine::updateInputModeAction(InputContext *ic) {
    for (size_t i = 0; i < inputModeSubAction_.size(); i++) {
        inputModeSubAction_[i]->setChecked(
            i == static_cast<size_t>(*config_.inputMode));
        inputModeSubAction_[i]->update(ic);
    }
    inputModeAction_->setLongText(
        InputModeI18NAnnotation::toString(*config_.inputMode));
    inputModeAction_->update(ic);
}

bool DirectCommit::preKeyEvent(KeyEvent &keyEvent,
                               const std::function<void()> &commit) {
    if (handleFakeBackspace(keyEvent)) {
        return true;
    }
    if (modeListOpen_) {
        handleModeListKey(keyEvent);
        return true;
    }
    if (!keyEvent.isRelease() &&
        keyEvent.key().checkKeyList(*engine_->config().inputModeSwitchKey)) {
        commit();
        openModeList();
        keyEvent.filterAndAccept();
        return true;
    }
    InputMode mode;
    return !resolveMode(&mode);
}

void UnikeyEngine::initInputModeAction() {
    auto &uiManager = instance_->userInterfaceManager();
    inputModeAction_ = std::make_unique<SimpleAction>();
    inputModeAction_->setShortText(_("Input mode"));
    inputModeAction_->setIcon("edit-select-text");
    uiManager.registerAction("unikey-input-mode", inputModeAction_.get());
    inputModeMenu_ = std::make_unique<Menu>();
    inputModeAction_->setMenu(inputModeMenu_.get());

    for (InputMode mode :
         {InputMode::Preedit, InputMode::SurroundingText,
          InputMode::ForwardKeyEventI, InputMode::ForwardKeyEventII,
          InputMode::ForwardAsCommit, InputMode::XTestFakeKeyEvent,
          InputMode::Off}) {
        inputModeSubAction_.emplace_back(std::make_unique<SimpleAction>());
        auto *action = inputModeSubAction_.back().get();
        action->setShortText(InputModeI18NAnnotation::toString(mode));
        action->setCheckable(true);
        connections_.emplace_back(action->connect<SimpleAction::Activated>(
            [this, mode](InputContext *ic) {
                // Finish the current word with the old mode before switching.
                commitCurrent(ic);
                config_.inputMode.setValue(mode);
                safeSaveAsIni(config_, "conf/unikey.conf");
                updateInputModeAction(ic);
            }));
        uiManager.registerAction("unikey-input-mode-" +
                                     std::to_string(static_cast<int>(mode)),
                                 action);
        inputModeMenu_->addAction(action);
    }
}

void UnikeyEngine::migrateInputModeConfig() {
    // Migrate the old DirectCommit and DirectCommitMethod options.
    RawConfig raw;
    readAsIni(raw, StandardPathsType::PkgConfig, "conf/unikey.conf");
    if (!raw.valueByPath("InputMode") && raw.valueByPath("DirectCommit") &&
        *raw.valueByPath("DirectCommit") == "True") {
        const auto *method = raw.valueByPath("DirectCommitMethod");
        config_.inputMode.setValue(method && *method == "XTest fake key event"
                                       ? InputMode::XTestFakeKeyEvent
                                       : InputMode::ForwardKeyEventI);
    }
}

} // namespace fcitx
