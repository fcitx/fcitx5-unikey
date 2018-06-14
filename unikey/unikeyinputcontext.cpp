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
#include "unikeyinputcontext.h"
#include "ukengine.h"
#include "usrkeymap.h"
#include <ctype.h>
#include <iostream>
#include <memory.h>
#include <stdio.h>

using namespace std;

//--------------------------------------------
void CreateDefaultUnikeyOptions(UnikeyOptions *pOpt) {
    pOpt->freeMarking = 1;
    pOpt->modernStyle = 0;
    pOpt->macroEnabled = 0;
    pOpt->useUnicodeClipboard = 0;
    pOpt->alwaysMacro = 0;
    pOpt->spellCheckEnabled = 1;
    pOpt->autoNonVnRestore = 0;
}

UnikeyInputMethod::UnikeyInputMethod()
    : sharedMem_(std::make_unique<UkSharedMem>()) {
    SetupUnikeyEngine();
    sharedMem_->input.init();
    sharedMem_->macStore.init();
    sharedMem_->vietKey = 1;
    sharedMem_->usrKeyMapLoaded = 0;
    setInputMethod(UkTelex);
    setOutputCharset(CONV_CHARSET_XUTF8);
    sharedMem_->initialized = 1;
    CreateDefaultUnikeyOptions(&sharedMem_->options);
}

//--------------------------------------------
void UnikeyInputMethod::setInputMethod(UkInputMethod im) {
    if (im == UkTelex || im == UkVni || im == UkSimpleTelex ||
        im == UkSimpleTelex2 || im == UkViqr || im == UkMsVi) {
        sharedMem_->input.setIM(im);
    } else if (im == UkUsrIM && sharedMem_->usrKeyMapLoaded) {
        // cout << "Switched to user mode\n"; //DEBUG
        sharedMem_->input.setIM(sharedMem_->usrKeyMap);
    }
    emit<Reset>();
    // cout << "IM changed to: " << im << endl; //DEBUG
}

void UnikeyInputMethod::setOutputCharset(int charset) {
    sharedMem_->charsetId = charset;
    emit<Reset>();
}

//--------------------------------------------
void UnikeyInputMethod::setOptions(UnikeyOptions *pOpt) {
    sharedMem_->options.freeMarking = pOpt->freeMarking;
    sharedMem_->options.modernStyle = pOpt->modernStyle;
    sharedMem_->options.macroEnabled = pOpt->macroEnabled;
    sharedMem_->options.useUnicodeClipboard = pOpt->useUnicodeClipboard;
    sharedMem_->options.alwaysMacro = pOpt->alwaysMacro;
    sharedMem_->options.spellCheckEnabled = pOpt->spellCheckEnabled;
    sharedMem_->options.autoNonVnRestore = pOpt->autoNonVnRestore;
}

//--------------------------------------------
void UnikeyInputContext::setCapsState(int shiftPressed, int CapsLockOn) {
    // UnikeyCapsAll = (shiftPressed && !CapsLockOn) || (!shiftPressed &&
    // CapsLockOn);
    capsLockOn_ = CapsLockOn;
    shiftPressed_ = shiftPressed;
}

//--------------------------------------------
UnikeyInputContext::UnikeyInputContext(UnikeyInputMethod *im) {
    conn_ =
        im->connect<UnikeyInputMethod::Reset>([this]() { engine_.reset(); });
    engine_.setCtrlInfo(im->sharedMem());
    engine_.setCheckKbCaseFunc([this](int *pShiftPressed, int *pCapsLockOn) {
        *pShiftPressed = shiftPressed_;
        *pCapsLockOn = capsLockOn_;
    });
}

//--------------------------------------------
UnikeyInputContext::~UnikeyInputContext() {}

//--------------------------------------------
void UnikeyInputContext::filter(unsigned int ch) {
    bufChars_ = sizeof(buf_);
    engine_.process(ch, backspaces_, buf_, bufChars_, output_);
}

//--------------------------------------------
void UnikeyInputContext::putChar(unsigned int ch) {
    engine_.pass(ch);
    bufChars_ = 0;
    backspaces_ = 0;
}

//--------------------------------------------
void UnikeyInputContext::resetBuf() { engine_.reset(); }

//--------------------------------------------
void UnikeyInputContext::backspacePress() {
    bufChars_ = sizeof(buf_);
    engine_.processBackspace(backspaces_, buf_, bufChars_, output_);
    //  printf("Backspaces: %d\n",UnikeyBackspaces);
}

//--------------------------------------------
void UnikeyInputContext::restoreKeyStrokes() {
    bufChars_ = sizeof(buf_);
    engine_.restoreKeyStrokes(backspaces_, buf_, bufChars_, output_);
}

bool UnikeyInputContext::isAtWordBeginning() {
    return engine_.atWordBeginning();
}
