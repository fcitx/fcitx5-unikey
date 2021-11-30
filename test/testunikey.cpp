/*
 * SPDX-FileCopyrightText: 2021-2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "testdir.h"
#include "testfrontend_public.h"
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/instance.h>
#include <iostream>

using namespace fcitx;

void scheduleEvent(EventDispatcher *dispatcher, Instance *instance) {
    dispatcher->schedule([dispatcher, instance]() {
        auto *unikey = instance->addonManager().addon("unikey", true);
        FCITX_ASSERT(unikey);
        auto defaultGroup = instance->inputMethodManager().currentGroup();
        defaultGroup.inputMethodList().clear();
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("keyboard-us"));
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("unikey"));
        defaultGroup.setDefaultInputMethod("");
        instance->inputMethodManager().setGroup(defaultGroup);
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ăo ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("âo ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ăo ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ăo");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("awoo ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("awo");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("aao");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("aao");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("aao");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("aao");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("aao");
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Control+space"),
                                                    false);
        RawConfig config;
        config.setValueByPath("SpellCheck", "False");
        unikey->setConfig(config);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("w"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("space"), false);

        config.setValueByPath("Macro", "False");
        unikey->setConfig(config);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("b"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("space"), false);

        config.setValueByPath("AutoNonVnRestore", "True");
        unikey->setConfig(config);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("w"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("space"), false);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("w"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        config.setValueByPath("SpellCheck", "True");
        unikey->setConfig(config);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("w"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("space"), false);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("w"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("Return"), false);

        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("a"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key("o"), false);
        testfrontend->call<ITestFrontend::keyEvent>(uuid, Key(FcitxKey_ssharp),
                                                    false);

        dispatcher->schedule([dispatcher, instance]() {
            dispatcher->detach();
            instance->exit();
        });
    });
}

void runInstance() {}

int main() {
    setupTestingEnvironment(TESTING_BINARY_DIR, {TESTING_BINARY_DIR "/src"},
                            {TESTING_BINARY_DIR "/test"});
    // fcitx::Log::setLogRule("default=5,table=5,libime-table=5");
    char arg0[] = "testunikey";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend,unikey";
    char *argv[] = {arg0, arg1, arg2};
    fcitx::Log::setLogRule("default=5,unikey=5");
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    EventDispatcher dispatcher;
    dispatcher.attach(&instance.eventLoop());
    scheduleEvent(&dispatcher, &instance);
    instance.exec();

    return 0;
}
