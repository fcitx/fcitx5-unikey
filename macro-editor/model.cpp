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
#include <QApplication>

#include "editor.h"
#include "model.h"
#include <fcitx-utils/i18n.h>

namespace fcitx {
namespace unikey {

typedef QPair<QString, QString> ItemType;

MacroModel::MacroModel(QObject *parent)
    : QAbstractTableModel(parent), needSave_(false) {}

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

int MacroModel::rowCount(const QModelIndex &) const { return list_.count(); }

int MacroModel::columnCount(const QModelIndex &) const { return 2; }

QVariant MacroModel::data(const QModelIndex &index, int role) const {
    do {
        if (role == Qt::DisplayRole && index.row() < list_.count()) {
            if (index.column() == 0) {
                return list_[index.row()].first;
            } else if (index.column() == 1) {
                return list_[index.row()].second;
            }
        }
    } while (0);
    return QVariant();
}

void MacroModel::addItem(const QString &macro, const QString &word) {
    if (keyset_.contains(macro))
        return;
    beginInsertRows(QModelIndex(), list_.size(), list_.size());
    list_.append(QPair<QString, QString>(macro, word));
    keyset_.insert(macro);
    endInsertRows();
    setNeedSave(true);
}

void MacroModel::deleteItem(int row) {
    if (row >= list_.count())
        return;
    QPair<QString, QString> item = list_.at(row);
    QString key = item.first;
    beginRemoveRows(QModelIndex(), row, row);
    list_.removeAt(row);
    keyset_.remove(key);
    endRemoveRows();
    setNeedSave(true);
}

void MacroModel::deleteAllItem() {
    if (list_.count())
        setNeedSave(true);
    beginResetModel();
    list_.clear();
    keyset_.clear();
    endResetModel();
}

void MacroModel::setNeedSave(bool needSave) {
    if (needSave_ != needSave) {
        needSave_ = needSave;
        emit needSaveChanged(needSave_);
    }
}

bool MacroModel::needSave() { return needSave_; }

void MacroModel::load(CMacroTable *table) {
    beginResetModel();
    list_.clear();
    keyset_.clear();
    for (int i = 0; i < table->getCount(); i++) {
        QString key = MacroEditor::getData(table, i, true);
        QString value = MacroEditor::getData(table, i, false);
        list_.append(QPair<QString, QString>(key, value));
        keyset_.insert(key);
    }
    endResetModel();
}

void MacroModel::save(CMacroTable *table) {
    table->resetContent();
    foreach(const ItemType &item, list_) {
        table->addItem(item.first.toUtf8().data(), item.second.toUtf8().data(),
                       CONV_CHARSET_XUTF8);
    }
    setNeedSave(false);
}

} // namespace unikey
} // namespace fcitx
