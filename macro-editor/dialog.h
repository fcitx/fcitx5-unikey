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
#ifndef _MACRO_EDITOR_DIALOG_H_
#define _MACRO_EDITOR_DIALOG_H_

#include "ui_dialog.h"
#include <QDialog>

class CMacroTable;

namespace fcitx {
namespace unikey {
class MacroDialog : public QDialog, private Ui::Dialog {
    Q_OBJECT
public:
    explicit MacroDialog(QWidget *parent = nullptr);
    QString macro() const;
    QString word() const;
};
} // namespace unikey
} // namespace fcitx

#endif // _MACRO_EDITOR_DIALOG_H_
