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
#ifndef _UNIKEY_UNIKEYINPUTCONTEXT_H_
#define _UNIKEY_UNIKEYINPUTCONTEXT_H_

#include "keycons.h"
#include "ukengine.h"
#include <memory>
#include <fcitx-utils/connectableobject.h>

class UnikeyInputMethod : public fcitx::ConnectableObject {
public:
    UnikeyInputMethod();

    // set input method
    //   im: TELEX_INPUT, VNI_INPUT, VIQR_INPUT, VIQR_STAR_INPUT
    void setInputMethod(UkInputMethod im);
    // set output format
    void setOutputCharset(int charset);

    // set extra options
    void setOptions(UnikeyOptions *pOpt);

    //--------------------------------------------
    int loadMacroTable(const char *fileName) {
        return sharedMem_->macStore.loadFromFile(fileName);
    }

    UkSharedMem *sharedMem() { return sharedMem_.get(); }

    FCITX_DECLARE_SIGNAL(UnikeyInputMethod, Reset, void());

private:
    FCITX_DEFINE_SIGNAL(UnikeyInputMethod, Reset);
    std::unique_ptr<UkSharedMem> sharedMem_;;
};

class UnikeyInputContext {
public:
    UnikeyInputContext(UnikeyInputMethod *im);
    ~UnikeyInputContext();

    // call this to reset Unikey's state when focus, context is changed or
    // some control key is pressed
    void resetBuf();

    // main handler, call every time a character input is received
    void filter(unsigned int ch);
    void putChar(unsigned int ch); // put new char without filtering

    // call this before UnikeyFilter for correctly processing some TELEX shortcuts
    void setCapsState(int shiftPressed, int CapsLockOn);

    // call this when backspace is pressed
    void backspacePress();

    // call this to restore to original key strokes
    void restoreKeyStrokes();

    int loadMacroTable(const char *fileName);

    bool isAtWordBeginning();

    int backspaces() const { return backspaces_; }
    int bufChars() const { return bufChars_; }
    const unsigned char *buf() const { return buf_; }

private:
    fcitx::ScopedConnection conn_;

    unsigned char buf_[1024];
    int backspaces_ = 0;
    int bufChars_;
    UkOutputType output_;
    UkEngine engine_;

    int capsLockOn_ = 0;
    int shiftPressed_ = 0;
};

#endif // _UNIKEY_UNIKEYINPUTCONTEXT_H_

