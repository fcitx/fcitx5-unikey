/*
 * SPDX-FileCopyrightText: 2012-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "main.h"
#include "editor.h"
#include "model.h"
#include <QApplication>
#include <qplugin.h>

namespace fcitx {

MacroEditorPlugin::MacroEditorPlugin(QObject *parent)
    : FcitxQtConfigUIPlugin(parent) {
    registerDomain("fcitx5-unikey", FCITX_INSTALL_LOCALEDIR);
}

FcitxQtConfigUIWidget *MacroEditorPlugin::create(const QString &key) {
    Q_UNUSED(key);
    return new fcitx::unikey::MacroEditor;
}

} // namespace fcitx
