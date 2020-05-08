/*
 * SPDX-FileCopyrightText: 2012-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _MACRO_EDITOR_EDITOR_H_
#define _MACRO_EDITOR_EDITOR_H_

#include "ui_editor.h"
#include <fcitxqtconfiguiwidget.h>

class CMacroTable;

namespace fcitx {
namespace unikey {

class MacroModel;
class MacroEditor : public FcitxQtConfigUIWidget, public Ui::Editor {
    Q_OBJECT
public:
    explicit MacroEditor(QWidget *parent = 0);
    virtual ~MacroEditor();
    void load() override;
    void save() override;
    QString title() override;
    QString icon() override;

    static QString getData(CMacroTable *table, int i, bool iskey);
private slots:
    void addWord();
    void deleteWord();
    void deleteAllWord();
    void itemFocusChanged();
    void addWordAccepted();
    void importMacro();
    void exportMacro();
    void importFileSelected();
    void exportFileSelected();

private:
    CMacroTable *table_;
    MacroModel *model_;
};
} // namespace unikey
} // namespace fcitx

#endif // _MACRO_EDITOR_EDITOR_H_
