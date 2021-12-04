/*
 * SPDX-FileCopyrightText: 2018-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _FCITX5_UNIKEY_UNIKEY_CONFIG_H_
#define _FCITX5_UNIKEY_UNIKEY_CONFIG_H_

#include <fcitx-config/configuration.h>
#include <fcitx-config/enum.h>
#include <fcitx-utils/i18n.h>
#include <keycons.h>

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
    ExternalOption macroEditor{this, "MacroEditor", _("Macro Editor"),
                               "fcitx://config/addon/unikey/macro"};);

} // namespace fcitx

#endif // _FCITX5_UNIKEY_UNIKEY_CONFIG_H_
