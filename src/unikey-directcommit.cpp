/*
 * SPDX-FileCopyrightText: 2026 Owen
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "unikey-directcommit.h"
#include "unikey-config.h"
#include "unikey-im.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/event.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/userinterface.h>
#include <string>
#include <string_view>
#include <utility>
#include <xcb/xcb.h>
#include <xcb/xtest.h>

namespace fcitx {

DirectCommit::DirectCommit(UnikeyEngine *engine, InputContext *ic,
                           std::function<void(KeyEvent &)> processKey)
    : engine_(engine), ic_(ic), processKey_(std::move(processKey)) {}

DirectCommit::~DirectCommit() = default;

void DirectCommit::keyEvent(KeyEvent &keyEvent) {
    if (waiting_) {
        queueKey(keyEvent.rawKey(), keyEvent.isRelease(), false);
        keyEvent.filterAndAccept();
        return;
    }
    processKey_(keyEvent);
    // The key would go to the client before the pending commit.
    if (waiting_ && !keyEvent.filtered()) {
        queueKey(keyEvent.rawKey(), keyEvent.isRelease(), true);
        keyEvent.filterAndAccept();
    }
}

bool DirectCommit::handleFakeBackspace(const KeyEvent &keyEvent) {
    if (fakeBackspace_ <= 0 || keyEvent.rawKey().sym() != FcitxKey_BackSpace) {
        return false;
    }
    if (keyEvent.isRelease() && --fakeBackspace_ == 0) {
        // Give the client a moment to apply the last BackSpace.
        armTimer(5000);
    }
    return true;
}

void DirectCommit::queueKey(const Key &key, bool isRelease, bool processed) {
    QueuedKey item{key, isRelease, processed};
    if (replaying_) {
        queue_.insert(queue_.begin() + replayInsert_++, item);
    } else {
        queue_.push_back(item);
    }
}

bool DirectCommit::passBackspace(const std::string &text) {
    if (!enabled() || waiting_ ||
        utf8::length(text) + 1 != utf8::length(committedStr_) ||
        committedStr_.compare(0, text.size(), text) != 0) {
        return false;
    }
    committedStr_ = text;
    return true;
}

void DirectCommit::commit(const std::string &text) {
    if (waiting_ && !replaying_) {
        // Focus change or similar, don't wait any longer.
        flush();
    }
    update(text);
    committedStr_.clear();
}

void DirectCommit::reset() {
    if (modeListOpen_) {
        closeModeList();
    }
    committedStr_.clear();
}

void DirectCommit::update(const std::string &text) {
    ic_->inputPanel().reset();
    ic_->updatePreedit();
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);

    // Find the common prefix of committed and new text in whole utf8 chars.
    auto committedEnd = committedStr_.end();
    auto newIter = text.begin();
    auto oldIter = committedStr_.begin();
    while (oldIter != committedEnd && newIter != text.end()) {
        auto oldNext = utf8::nextChar(oldIter);
        auto newNext = utf8::nextChar(newIter);
        if (std::string_view(&*oldIter, oldNext - oldIter) !=
            std::string_view(&*newIter, newNext - newIter)) {
            break;
        }
        oldIter = oldNext;
        newIter = newNext;
    }
    auto toDelete = utf8::length(oldIter, committedEnd);
    std::string toCommit(newIter, text.end());
    committedStr_ = text;
    if (toDelete == 0 && waiting_) {
        pendingCommit_ += toCommit;
        return;
    }
    if (toDelete == 0) {
        if (!toCommit.empty()) {
            ic_->commitString(toCommit);
        }
        return;
    }

    if (waiting_) {
        // Should not happen since keys are queued while waiting.
        flush();
    }
    const uint64_t delay = *engine_->config().directCommitDelay * 1000ULL;
    auto mode = inputMode();
    if (mode == InputMode::SurroundingText) {
        if (ic_->capabilityFlags().test(CapabilityFlag::SurroundingText)) {
            ic_->deleteSurroundingText(-static_cast<int>(toDelete), toDelete);
            if (!toCommit.empty()) {
                ic_->commitString(toCommit);
            }
            return;
        }
        // Client doesn't support it, fall back to forwarding keys.
        mode = InputMode::ForwardKeyEventI;
    }
    if (mode == InputMode::ForwardAsCommit) {
        // Terminals turn "\b" into ^H which the shell treats as BackSpace.
        ic_->commitString(std::string(toDelete, '\b') + toCommit);
        return;
    }
    if (mode == InputMode::ForwardKeyEventII) {
        // Select the old text, the commit replaces the selection.
        const Key left(FcitxKey_Left, KeyState::Shift, 113);
        ic_->forwardKey(Key(FcitxKey_Shift_L, KeyStates(), 50), false);
        for (size_t i = 0; i < toDelete; i++) {
            ic_->forwardKey(left, false);
            ic_->forwardKey(left, true);
        }
        ic_->forwardKey(Key(FcitxKey_Shift_L, KeyState::Shift, 50), true);
        if (toCommit.empty()) {
            forwardBackspace(1);
        } else {
            ic_->commitString(toCommit);
        }
        return;
    }
    if (delay == 0 && mode != InputMode::XTestFakeKeyEvent) {
        // Client applies forwarded keys and commits in order, no need to wait.
        forwardBackspace(toDelete);
        if (!toCommit.empty()) {
            ic_->commitString(toCommit);
        }
        return;
    }
    // Like ibus-bamboo, the new text must not be committed before the
    // client has applied the BackSpace, otherwise the order gets mixed up.
    pendingCommit_ = std::move(toCommit);
    waiting_ = true;
    if (mode == InputMode::XTestFakeKeyEvent &&
        engine_->sendFakeBackspace(toDelete)) {
        fakeBackspace_ = toDelete;
        // Fallback in case some BackSpace never comes back to us.
        armTimer(delay + 300000);
        return;
    }
    forwardBackspace(toDelete);
    armTimer(delay + toDelete * 5000);
}

void DirectCommit::forwardBackspace(size_t n) {
    // Clients drop forwarded keys without a keycode, so pass the xkb keycode
    // of BackSpace (evdev 14 + 8).
    const Key backspace(FcitxKey_BackSpace, KeyStates(), 22);
    for (size_t i = 0; i < n; i++) {
        ic_->forwardKey(backspace, false);
        ic_->forwardKey(backspace, true);
    }
}

void DirectCommit::armTimer(uint64_t usec) {
    auto when = now(CLOCK_MONOTONIC) + usec;
    if (timer_) {
        timer_->setTime(when);
        timer_->setOneShot();
        return;
    }
    timer_ = engine_->instance()->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, when, 0, [this](EventSourceTime *, uint64_t) {
            flush();
            return true;
        });
}

void DirectCommit::flush() {
    if (!waiting_) {
        return;
    }
    waiting_ = false;
    fakeBackspace_ = 0;
    if (timer_) {
        timer_->setEnabled(false);
    }
    if (!pendingCommit_.empty()) {
        ic_->commitString(pendingCommit_);
        pendingCommit_.clear();
    }
    // A replayed key may start waiting again; keys it queues go in front of
    // the remaining ones to keep the order.
    replaying_ = true;
    while (!waiting_ && !queue_.empty()) {
        auto item = queue_.front();
        queue_.pop_front();
        replayInsert_ = 0;
        bool filtered = false;
        if (!item.processed) {
            KeyEvent event(ic_, item.key, item.isRelease);
            keyEvent(event);
            filtered = event.filtered();
        }
        if (filtered) {
            continue;
        }
        auto chr = Key::keySymToUnicode(item.key.sym());
        if (!item.isRelease && chr >= 0x20 && chr != 0x7f &&
            !item.key.states().testAny(
                KeyStates{KeyState::Ctrl, KeyState::Alt, KeyState::Super})) {
            // Plain text is committed so it stays ordered with our commits.
            ic_->commitString(utf8::UCS4ToUTF8(chr));
        } else {
            ic_->forwardKey(item.key, item.isRelease);
        }
    }
    replaying_ = false;
}

void UnikeyEngine::XcbDisconnect::operator()(xcb_connection_t *conn) const {
    xcb_disconnect(conn);
}

bool UnikeyEngine::sendFakeBackspace(int n) {
    if (!xtestConn_) {
        const char *display = std::getenv("DISPLAY");
        if (!display || !*display) {
            return false;
        }
        xtestConn_.reset(xcb_connect(display, nullptr));
        if (xcb_connection_has_error(xtestConn_.get())) {
            xtestConn_.reset();
            return false;
        }
    }
    // X keycode of BackSpace (evdev 14 + 8).
    constexpr uint8_t backspace = 22;
    for (int i = 0; i < n; i++) {
        xcb_test_fake_input(xtestConn_.get(), XCB_KEY_PRESS, backspace,
                            XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
        xcb_test_fake_input(xtestConn_.get(), XCB_KEY_RELEASE, backspace,
                            XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
    }
    xcb_flush(xtestConn_.get());
    return true;
}

} // namespace fcitx
