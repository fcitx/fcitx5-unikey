//
// Copyright (C) 2012~2018 by CSSlayer
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
#include "dialog.h"
#include "ui_dialog.h"

namespace fcitx {
namespace unikey {
MacroDialog::MacroDialog(QWidget *parent) : QDialog(parent) { setupUi(this); }

QString MacroDialog::macro() const { return macroLineEdit->text(); }

QString MacroDialog::word() const { return wordLineEdit->text(); }

} // namespace unikey
} // namespace fcitx
