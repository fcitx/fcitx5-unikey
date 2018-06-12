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

#include <fcitxqtconfiguiwidget.h>

class CMacroTable;
namespace Ui {
class Editor;
}

namespace fcitx {
namespace unikey {

class MacroModel;
class MacroEditor : public FcitxQtConfigUIWidget {
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
    Ui::Editor *m_ui;
    CMacroTable *m_table;
    MacroModel *m_model;
};
} // namespace unikey
} // namespace fcitx
