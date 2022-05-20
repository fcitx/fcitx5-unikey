/*
 * SPDX-FileCopyrightText: 2012-2018 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include <QCloseEvent>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include "dialog.h"
#include "editor.h"
#include "mactab.h"
#include "model.h"
#include "ui_editor.h"
#include <fcitx-utils/standardpath.h>

namespace fcitx {
namespace unikey {

MacroEditor::MacroEditor(QWidget *parent)
    : FcitxQtConfigUIWidget(parent), table_(std::make_unique<CMacroTable>()),
      model_(new MacroModel(this)) {
    setupUi(this);

    connect(addButton, &QPushButton::clicked, this, &MacroEditor::addWord);
    connect(deleteButton, &QPushButton::clicked, this,
            &MacroEditor::deleteWord);
    connect(clearButton, &QPushButton::clicked, this,
            &MacroEditor::deleteAllWord);
    connect(importButton, &QPushButton::clicked, this,
            &MacroEditor::importMacro);
    connect(exportButton, &QPushButton::clicked, this,
            &MacroEditor::exportMacro);
    table_->init();
    macroTableView->horizontalHeader()->setStretchLastSection(true);
    macroTableView->verticalHeader()->setVisible(false);
    macroTableView->setModel(model_);
    connect(macroTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &MacroEditor::itemFocusChanged);
    connect(model_, &MacroModel::needSaveChanged, this, &MacroEditor::changed);
    load();
    itemFocusChanged();
}

MacroEditor::~MacroEditor() {}

QString MacroEditor::icon() { return "fcitx-unikey"; }

QString MacroEditor::title() { return _("Unikey Macro Editor"); }

void MacroEditor::itemFocusChanged() {
    deleteButton->setEnabled(macroTableView->currentIndex().isValid());
}

void MacroEditor::deleteWord() {
    if (!macroTableView->currentIndex().isValid()) {
        return;
    }
    int row = macroTableView->currentIndex().row();
    model_->deleteItem(row);
}

void MacroEditor::deleteAllWord() { model_->deleteAllItem(); }

void MacroEditor::addWord() {
    MacroDialog *dialog = new MacroDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->open();
    connect(dialog, &QDialog::accepted, this, &MacroEditor::addWordAccepted);
}

QString MacroEditor::getData(CMacroTable *table, int i, bool iskey) {

    char key[MAX_MACRO_KEY_LEN * 3];
    char value[MAX_MACRO_TEXT_LEN * 3];
    do {
        if (i < table->getCount()) {
            const StdVnChar *p = NULL;
            int maxOutLen = 0;
            const char *result = NULL;
            if (iskey) {
                p = table->getKey(i);
                maxOutLen = sizeof(key);
                result = key;
            } else {
                p = table->getText(i);
                maxOutLen = sizeof(value);
                result = value;
            }

            if (!p)
                break;
            int inLen = -1;
            int ret =
                VnConvert(CONV_CHARSET_VNSTANDARD, CONV_CHARSET_XUTF8,
                          (UKBYTE *)p, (UKBYTE *)result, &inLen, &maxOutLen);
            if (ret != 0)
                break;
            return QString::fromUtf8(result);
        }
    } while (0);
    return QString();
}

void MacroEditor::addWordAccepted() {
    const MacroDialog *dialog =
        qobject_cast<const MacroDialog *>(QObject::sender());

    model_->addItem(dialog->macro(), dialog->word());
}

void MacroEditor::load() {
    auto path = StandardPath::global().locate(StandardPath::Type::PkgConfig,
                                              "unikey/macro");
    table_->loadFromFile(path.data());
    model_->load(table_.get());
}

void MacroEditor::save() {
    model_->save(table_.get());
    StandardPath::global().safeSave(StandardPath::Type::PkgConfig,
                                    "unikey/macro", [this](int fd) -> bool {
                                        UnixFD unixFD(fd);
                                        auto f = fs::openFD(unixFD, "wb");
                                        return table_->writeToFp(f.release());
                                    });
}

void MacroEditor::importMacro() {
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    dialog->open();
    connect(dialog, &QFileDialog::accepted, this,
            &MacroEditor::importFileSelected);
}

void MacroEditor::importFileSelected() {
    const QFileDialog *dialog =
        qobject_cast<const QFileDialog *>(QObject::sender());
    if (dialog->selectedFiles().length() <= 0) {
        return;
    }
    QString file = dialog->selectedFiles()[0];
    table_->loadFromFile(file.toUtf8().constData());
}

void MacroEditor::exportMacro() {
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setDirectory("macro");
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->open();
    connect(dialog, &QFileDialog::accepted, this,
            &MacroEditor::exportFileSelected);
}

void MacroEditor::exportFileSelected() {
    const QFileDialog *dialog =
        qobject_cast<const QFileDialog *>(QObject::sender());
    if (dialog->selectedFiles().length() <= 0) {
        return;
    }
    QString file = dialog->selectedFiles()[0];
    table_->writeToFile(file.toUtf8().constData());
}

} // namespace unikey

} // namespace fcitx
