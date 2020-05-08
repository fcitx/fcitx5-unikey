/*
 * SPDX-FileCopyrightText: 2012-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _MACRO_EDITOR_MODEL_H_
#define _MACRO_EDITOR_MODEL_H_

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
    void save(CMacroTable *table);
    bool needSave();

signals:
    void needSaveChanged(bool);

private:
    void setNeedSave(bool needSave);
    bool needSave_;
    QSet<QString> keyset_;
    QList<QPair<QString, QString>> list_;
};
} // namespace unikey
} // namespace fcitx

#endif // _MACRO_EDITOR_MODEL_H_
