/*
 * SPDX-FileCopyrightText: 2012-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#ifndef _KEYMAP_EDITOR_MODEL_H_
#define _KEYMAP_EDITOR_MODEL_H_

#include "mactab.h"
#include "usrkeymap.h"
#include <QAbstractItemModel>
#include <QSet>

namespace fcitx {
namespace unikey {
class KeymapModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit KeymapModel(QObject *parent = 0);
    virtual ~KeymapModel();

    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    void load();
    QModelIndex addItem(unsigned char key, int action);
    void moveUp(int index);
    void moveDown(int index);
    void deleteItem(int row);
    void deleteAllItem();
    void save();
    bool needSave();
    void load(const QString &fileName);
    void save(const QString &fileName);
    void load(int profile);

signals:
    void needSaveChanged(bool);

private:
    void setNeedSave(bool needSave);
    bool saveToFd(int fd);
    bool needSave_;
    std::vector<UkKeyMapping> list_;
};
} // namespace unikey
} // namespace fcitx

#endif // _MACRO_EDITOR_MODEL_H_
