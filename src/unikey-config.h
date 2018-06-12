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
    Option<bool> spellCheck{this, "SpellCheck", _("Enable spell check"), false};
    Option<bool> macro{this, "Macro", "Enable Macro", true};
    Option<bool> process_w_at_begin{this, "ProcessWAtBegin",
                                    _("Process W at word begin"), true};
    Option<bool> autoNonVnRestore{this, "AutoNonVnRestore",
                                  _("Auto restore keys with invalid words"),
                                  false};
    Option<bool> modernStyle{this, "ModernStyle",
                             _("Use oà, _uý (instead of òa, úy)"), false};
    Option<bool> freeMarking{this, "FreeMarking",
                             _("Allow type with more freedom"), true};);

} // namespace fcitx

#endif // _FCITX5_UNIKEY_UNIKEY_CONFIG_H_
