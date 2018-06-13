/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#include "main.h"
#include "editor.h"
#include "model.h"
#include <QApplication>
#include <qplugin.h>

namespace fcitx {

MacroEditorPlugin::MacroEditorPlugin(QObject *parent)
    : FcitxQtConfigUIPlugin(parent) {}

FcitxQtConfigUIWidget *MacroEditorPlugin::create(const QString &key) {
    Q_UNUSED(key);
    return new fcitx::unikey::MacroEditor;
}

} // namespace fcitx
