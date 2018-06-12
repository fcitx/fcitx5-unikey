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

#include "mactab.h"
#include <QAbstractItemModel>
#include <QSet>

namespace fcitx {
namespace unikey {
class MacroModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit MacroModel(QObject *parent = 0);
    virtual ~MacroModel();

    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    void load(CMacroTable *table);
    void addItem(const QString &macro, const QString &word);
    void deleteItem(int row);
    void deleteAllItem();
    void save(CMacroTable *m_table);
    bool needSave();

signals:
    void needSaveChanged(bool m_needSave);

private:
    void setNeedSave(bool needSave);
    bool m_needSave;
    QSet<QString> m_keyset;
    QList<QPair<QString, QString>> m_list;
};
} // namespace unikey
} // namespace fcitx
