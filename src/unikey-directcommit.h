/*
 * SPDX-FileCopyrightText: 2026 Owen
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_UNIKEY_UNIKEY_DIRECTCOMMIT_H_
#define _FCITX5_UNIKEY_UNIKEY_DIRECTCOMMIT_H_

#include "unikey-config.h"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <fcitx-utils/eventloopinterface.h>
#include <fcitx-utils/key.h>
#include <fcitx/event.h>
#include <functional>
#include <memory>
#include <string>

namespace fcitx {

class InputContext;
class UnikeyEngine;

// Keywords of the per app mode, the index is the InputMode value.
// AppModeGlobal means the app has no entry and uses the global input mode.
inline constexpr const char *AppModeNames[] = {
    "preedit",         "surroundingtext", "forwardkeyevent1", "forwardkeyevent2",
    "forwardascommit", "xtest",           "off"};
inline constexpr int AppModeOff = static_cast<int>(InputMode::Off);
inline constexpr int AppModeGlobal = 7;

// Commits text directly and fixes it afterwards with one of the ibus-bamboo
// like input modes, instead of showing it as preedit. Also handles the per
// app input mode and the popup to choose it. One per input context.
class DirectCommit {
public:
    // processKey handles a key with unikey.
    DirectCommit(UnikeyEngine *engine, InputContext *ic,
                 std::function<void(KeyEvent &)> processKey);
    ~DirectCommit();

    // Resolve the input mode for this app, the per app mode overrides the
    // global one. Returns false if Vietnamese is turned off for the app.
    bool resolveMode(InputMode *mode) const;
    InputMode inputMode() const;
    // Whether text is committed directly instead of shown as preedit.
    bool enabled() const {
        auto mode = inputMode();
        return mode != InputMode::Preedit && mode != InputMode::Off;
    }

    // Handle keys before unikey sees them: fake BackSpace sent by XTest, the
    // input mode popup and its shortcut, apps with Vietnamese turned off.
    // commit finishes the current word. Returns true if the key is done.
    bool preKeyEvent(KeyEvent &keyEvent, const std::function<void()> &commit);
    // Pass the key to processKey. While waiting for erased text to be applied
    // by the client, keys are held so they reach it after our commit.
    void keyEvent(KeyEvent &keyEvent);

    // Sync text of the current word to the client by committing the new part
    // and erasing the part that changed.
    void update(const std::string &text);
    // Returns true if a real BackSpace only removes the last char of the
    // current word, so it can go to the client as is. A fake BackSpace sent
    // while the same key is held down gets dropped by some clients.
    bool passBackspace(const std::string &text);
    // Finish the current word.
    void commit(const std::string &text);
    // Forget the current word, the text stays in the client.
    void reset();

    // Save mode (an index of AppModeNames, or AppModeGlobal to remove the
    // entry) for the current app.
    void selectMode(int mode);

private:
    bool handleFakeBackspace(const KeyEvent &keyEvent);
    void openModeList();
    void closeModeList();
    void handleModeListKey(KeyEvent &keyEvent);
    void armTimer(uint64_t usec);
    void forwardBackspace(size_t n);
    // Commit text held back by BackSpace and replay queued keys.
    void flush();
    void queueKey(const Key &key, bool isRelease, bool processed);

    UnikeyEngine *engine_;
    InputContext *ic_;
    std::function<void(KeyEvent &)> processKey_;
    // Text of the current word already committed.
    std::string committedStr_;
    // Text to commit once pending BackSpace are applied by the client.
    std::string pendingCommit_;
    bool waiting_ = false;
    int fakeBackspace_ = 0;
    struct QueuedKey {
        Key key;
        bool isRelease;
        // Already handled by unikey, only needs to be passed on.
        bool processed;
    };
    std::deque<QueuedKey> queue_;
    bool replaying_ = false;
    size_t replayInsert_ = 0;
    std::unique_ptr<EventSourceTime> timer_;
    bool modeListOpen_ = false;
};

} // namespace fcitx

#endif // _FCITX5_UNIKEY_UNIKEY_DIRECTCOMMIT_H_
