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

#include <QApplication>

#include "editor.h"
#include "model.h"
#include <fcitx-utils/i18n.h>

namespace fcitx {
namespace unikey {

typedef QPair<QString, QString> ItemType;

MacroModel::MacroModel(QObject *parent)
    : QAbstractTableModel(parent), m_needSave(false) {}

MacroModel::~MacroModel() {}

QVariant MacroModel::headerData(int section, Qt::Orientation orientation,
                                int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0)
            return _("Macro");
        else if (section == 1)
            return _("Word");
    }
    return QVariant();
}

int MacroModel::rowCount(const QModelIndex &) const { return m_list.count(); }

int MacroModel::columnCount(const QModelIndex &) const { return 2; }

QVariant MacroModel::data(const QModelIndex &index, int role) const {
    do {
        if (role == Qt::DisplayRole && index.row() < m_list.count()) {
            if (index.column() == 0) {
                return m_list[index.row()].first;
            } else if (index.column() == 1) {
                return m_list[index.row()].second;
            }
        }
    } while (0);
    return QVariant();
}

void MacroModel::addItem(const QString &macro, const QString &word) {
    if (m_keyset.contains(macro))
        return;
    beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
    m_list.append(QPair<QString, QString>(macro, word));
    m_keyset.insert(macro);
    endInsertRows();
    setNeedSave(true);
}

void MacroModel::deleteItem(int row) {
    if (row >= m_list.count())
        return;
    QPair<QString, QString> item = m_list.at(row);
    QString key = item.first;
    beginRemoveRows(QModelIndex(), row, row);
    m_list.removeAt(row);
    m_keyset.remove(key);
    endRemoveRows();
    setNeedSave(true);
}

void MacroModel::deleteAllItem() {
    if (m_list.count())
        setNeedSave(true);
    beginResetModel();
    m_list.clear();
    m_keyset.clear();
    endResetModel();
}

void MacroModel::setNeedSave(bool needSave) {
    if (m_needSave != needSave) {
        m_needSave = needSave;
        emit needSaveChanged(m_needSave);
    }
}

bool MacroModel::needSave() { return m_needSave; }

void MacroModel::load(CMacroTable *table) {
    beginResetModel();
    m_list.clear();
    m_keyset.clear();
    for (int i = 0; i < table->getCount(); i++) {
        QString key = MacroEditor::getData(table, i, true);
        QString value = MacroEditor::getData(table, i, false);
        m_list.append(QPair<QString, QString>(key, value));
        m_keyset.insert(key);
    }
    endResetModel();
}

void MacroModel::save(CMacroTable *m_table) {
    m_table->resetContent();
    foreach(const ItemType &item, m_list) {
        m_table->addItem(item.first.toUtf8().data(),
                         item.second.toUtf8().data(), CONV_CHARSET_XUTF8);
    }
    setNeedSave(false);
}

} // namespace unikey
} // namespace fcitx
