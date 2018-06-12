//
// Copyright (C) 2018~2018 by CSSlayer
// wengxt@gmail.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
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
        reloadConfig();
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
};

class UnikeyFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new UnikeyEngine(manager->instance());
    }
};
} // namespace fcitx

#endif // _FCITX5_UNIKEY_UNIKEY_IM_H_
