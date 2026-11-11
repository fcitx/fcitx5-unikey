/*
 * SPDX-FileCopyrightText: 2018-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_UNIKEY_UNIKEY_CONFIG_H_
#define _FCITX5_UNIKEY_UNIKEY_CONFIG_H_

#include "keycons.h"
#include <fcitx-config/configuration.h>
#include <fcitx-config/enum.h>
#include <fcitx-config/option.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>

namespace fcitx {

enum class UkConv {
    XUTF8,
    TCVN3,
    VNIWIN,
    VIQR,
    BKHCM2,
    UNI_CSTRING,
    UNIREF,
    UNIREF_HEX
};

FCITX_CONFIG_ENUM_NAME_WITH_I18N(UkConv, N_("Unicode"), N_("TCVN3"),
                                 N_("VNI Win"), N_("VIQR"), N_("BK HCM 2"),
                                 N_("CString"), N_("NCR Decimal"),
                                 N_("NCR Hex"));

FCITX_CONFIG_ENUM_NAME_WITH_I18N(UkInputMethod, N_("Telex"), N_("VNI"),
                                 N_("VIQR"), N_("Microsoft Vietnamese"),
                                 N_("UserIM"), N_("Simple Telex"),
                                 N_("Simple Telex2"));

enum class EmbeddedPreeditStyle { None, Underline, Highlight };

FCITX_CONFIG_ENUM_NAME_WITH_I18N(EmbeddedPreeditStyle, N_("None"),
                                 N_("Underline"), N_("Highlight"));

// Input modes, named after the ones of ibus-bamboo.
// Preedit: show the word as preedit, commit when it is done.
// The other modes commit directly and fix the committed text by:
// SurroundingText: asking the client to delete text before the cursor.
// ForwardKeyEventI: forwarding BackSpace to the client.
// ForwardKeyEventII: forwarding Shift+Left to select it, then typing over.
// ForwardAsCommit: committing "\b" characters, for terminals.
// XTestFakeKeyEvent: sending BackSpace via the X server (X11/XWayland only).
// Off: no Vietnamese, keys go to the client as is.
enum class InputMode {
    Preedit,
    SurroundingText,
    ForwardKeyEventI,
    ForwardKeyEventII,
    ForwardAsCommit,
    XTestFakeKeyEvent,
    Off
};

FCITX_CONFIG_ENUM_NAME_WITH_I18N(InputMode, N_("Pre-edit"),
                                 N_("Surrounding Text"),
                                 N_("ForwardKeyEvent I (BackSpace)"),
                                 N_("ForwardKeyEvent II (Shift+Left)"),
                                 N_("Forward as commit"),
                                 N_("XTestFakeKeyEvent"),
                                 N_("Turn off Vietnamese"));

FCITX_CONFIGURATION(
    UnikeyConfig,
    OptionWithAnnotation<UkInputMethod, UkInputMethodI18NAnnotation> im{
        this, "InputMethod", _("Input Method"), UkTelex};
    OptionWithAnnotation<UkConv, UkConvI18NAnnotation> oc{
        this, "OutputCharset", _("Output Charset"), UkConv::XUTF8};
    Option<bool> spellCheck{this, "SpellCheck", _("Enable spell check"), true};
    Option<bool> macro{this, "Macro", _("Enable Macro"), true};
    Option<bool> process_w_at_begin{this, "ProcessWAtBegin",
                                    _("Process W at word begin"), true};
    Option<bool> autoNonVnRestore{this, "AutoNonVnRestore",
                                  _("Auto restore keys with invalid words"),
                                  true};
    Option<bool> modernStyle{this, "ModernStyle",
                             _("Use oà, _uý (instead of òa, úy)"), false};
    Option<bool> freeMarking{this, "FreeMarking",
                             _("Allow type with more freedom"), true};
    Option<bool> surroundingText{
        this, "SurroundingText",
        _("Restore typing state from surrounding text"), true};
    Option<bool> modifySurroundingText{
        this, "ModifySurroundingText",
        _("Allow to modify surrounding text (experimental)"), false};
    OptionWithAnnotation<InputMode, InputModeI18NAnnotation> inputMode{
        this, "InputMode", _("Input mode"), InputMode::Preedit};
    KeyListOption inputModeSwitchKey{
        this,
        "InputModeSwitchKey",
        _("Choose input mode for current app"),
        {Key("Control+asciitilde")},
        KeyListConstrain({KeyConstrainFlag::AllowModifierLess})};
    KeyListOption restoreKey{
        this, "RestoreKey", _("Restore keys of the current word"), {},
        KeyListConstrain()};
    Option<int, IntConstrain> directCommitDelay{
        this, "DirectCommitDelay",
        _("Delay after backspace (ms)"), 0,
        IntConstrain(0, 500)};
    // Each entry is "app=mode", app is matched against the program name
    // (substring, case insensitive), mode is one of AppModeNames in
    // unikey-directcommit.h.
    Option<std::vector<std::string>, NoConstrain<std::vector<std::string>>,
           DefaultMarshaller<std::vector<std::string>>, ToolTipAnnotation>
        appModes{this,
                 "AppModes",
                 _("Per app mode"),
                 {},
                 {},
                 {},
                 {_("One entry per app in the form app=mode.\n"
                    "app is matched against the program name.\n"
                    "mode is one of: preedit, surroundingtext,\n"
                    "forwardkeyevent1, forwardkeyevent2, forwardascommit,\n"
                    "xtest, off (no Vietnamese).")}};
    Option<EmbeddedPreeditStyle> preeditStyle{this, "PreeditStyle",
                                              _("Style of the preedit text"),
                                              EmbeddedPreeditStyle::Underline};
#ifdef ENABLE_QT
    ExternalOption macroEditor{this, "MacroEditor", _("Macro Editor"),
                               "fcitx://config/addon/unikey/macro"};
    ExternalOption keymapEditor{this, "KeymapEditor", _("Keymap Editor"),
                                "fcitx://config/addon/unikey/keymap.txt"};
#endif
);
} // namespace fcitx

#endif // _FCITX5_UNIKEY_UNIKEY_CONFIG_H_
