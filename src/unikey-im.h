/*
 * SPDX-FileCopyrightText: 2018-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_UNIKEY_UNIKEY_IM_H_
#define _FCITX5_UNIKEY_UNIKEY_IM_H_

#include "unikey-config.h"
#include <fcitx-config/iniparser.h>
#include <fcitx/action.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <string>
#include <unikeyinputcontext.h>

namespace fcitx {

class UnikeyState;

class UnikeyEngine final : public InputMethodEngine {
public:
    UnikeyEngine(Instance *instance);
    ~UnikeyEngine();
    Instance *instance() { return instance_; }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, "conf/unikey.conf");
        populateConfig();
    }

    void setSubConfig(const std::string &path,
                      const fcitx::RawConfig &) override {
        if (path == "macro") {
            reloadMacroTable();
        } else if (path == "keymap.txt") {
            reloadKeymap();
            // Need to populate again if old keymap is not valid.
            populateConfig();
        }
    }

    void activate(const InputMethodEntry &entry,
                  InputContextEvent &event) override;
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    void reloadConfig() override;
    void reset(const InputMethodEntry &entry,
               InputContextEvent &event) override;
    void deactivate(const fcitx::InputMethodEntry &,
                    fcitx::InputContextEvent &event) override;
    void save() override;
    auto &factory() { return factory_; }
    auto &config() { return config_; }

    void updateUI(InputContext *ic);
    void updateMacroAction(InputContext *ic);
    void updateSpellAction(InputContext *ic);
    void updateCharsetAction(InputContext *ic);
    void updateInputMethodAction(InputContext *ic);

    std::string subMode(const InputMethodEntry &, InputContext &) override;

    UnikeyInputMethod *im() { return &im_; }

private:
    void populateConfig();
    void reloadMacroTable() {
        auto path = StandardPath::global().locate(StandardPath::Type::PkgConfig,
                                                  "unikey/macro");

        if (!path.empty()) {
            im_.loadMacroTable(path.data());
        }
    }
    void reloadKeymap();

    UnikeyConfig config_;
    UnikeyInputMethod im_;
    Instance *instance_;
    FactoryFor<UnikeyState> factory_;
    std::unique_ptr<SimpleAction> inputMethodAction_;
    std::vector<std::unique_ptr<SimpleAction>> inputMethodSubAction_;
    std::unique_ptr<Menu> inputMethodMenu_;
    std::unique_ptr<SimpleAction> charsetAction_;
    std::vector<std::unique_ptr<SimpleAction>> charsetSubAction_;
    std::unique_ptr<Menu> charsetMenu_;
    std::unique_ptr<SimpleAction> spellCheckAction_;
    std::unique_ptr<SimpleAction> macroAction_;
    std::vector<ScopedConnection> connections_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventWatchers_;
};

class UnikeyFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        registerDomain("fcitx5-unikey", FCITX_INSTALL_LOCALEDIR);
        return new UnikeyEngine(manager->instance());
    }
};
} // namespace fcitx

#endif // _FCITX5_UNIKEY_UNIKEY_IM_H_
