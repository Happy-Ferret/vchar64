/****************************************************************************
Copyright 2015 Ricardo Quesada

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
****************************************************************************/

#include "exportdialog.h"
#include "ui_exportdialog.h"

#include <QFileDialog>
#include <QSettings>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QStatusBar>

#include "mainwindow.h"
#include "state.h"

ExportDialog::ExportDialog(State* state, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog)
    , _settings()
    , _state(state)
    , _checkBox_clicked(State::EXPORT_FEATURE_CHARSET)
{
    ui->setupUi(this);

    auto lastDir = _settings.value(QLatin1String("dir/lastdir"), QDir::homePath()).toString();

    auto fn = _state->getExportedFilename();
    if (fn.length() == 0) {
        fn = state->getLoadedFilename();

        // if getExportedFilename() == 0 then getLoadedFilename() will have the .vcharproj extension.
        // we should replace it with .bin
        if (fn.length() != 0) {
            QFileInfo fileInfo(fn);
            fn = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".bin";
        }
    }
    if (fn.length() == 0)
        fn = lastDir + "/" + "untitled.bin";

    ui->editFilename->setText(fn);

    //
    connect(ui->radioButton_prg, &QRadioButton::toggled, [&](bool toogled){
        ui->spinBox_attribAddress->setEnabled(toogled);
        ui->spinBox_charsetAddress->setEnabled(toogled);
        ui->spinBox_mapAddress->setEnabled(toogled);
    });

    _checkBox_clicked = _state->getExportedFeatures();
    ui->checkBox_charset->setChecked(_checkBox_clicked & State::EXPORT_FEATURE_CHARSET);
    ui->checkBox_map->setChecked(_checkBox_clicked & State::EXPORT_FEATURE_MAP);
    ui->checkBox_attribs->setChecked(_checkBox_clicked & State::EXPORT_FEATURE_ATTRIBS);

    int format = _state->getExportedFormat();

    /* don't change the order */
    QRadioButton* radios[] = {
        ui->radioButton_raw,
        ui->radioButton_prg,
        ui->radioButton_asm
    };
    radios[format]->setChecked(true);
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

void ExportDialog::on_pushBrowse_clicked()
{
    auto filename = QFileDialog::getSaveFileName(this,
                                                 tr("Select filename"),
                                                 ui->editFilename->text(),
                                                 tr("Asm files (*.s *.a *.asm);;Raw files (*.raw *.bin);;PRG files (*.prg *.64c);;Any file (*)"),
                                                 nullptr,
                                                 QFileDialog::DontConfirmOverwrite);

    if (!filename.isEmpty())
        ui->editFilename->setText(filename);
}

void ExportDialog::accept()
{
    bool ok = false;
    auto filename = ui->editFilename->text();
    int whatToExport = State::EXPORT_FEATURE_NONE;

    if (ui->checkBox_map->isChecked())
        whatToExport |= State::EXPORT_FEATURE_MAP;
    if (ui->checkBox_attribs->isChecked())
        whatToExport |= State::EXPORT_FEATURE_ATTRIBS;
    if (ui->checkBox_charset->isChecked())
        whatToExport |= State::EXPORT_FEATURE_CHARSET;

    if (ui->radioButton_raw->isChecked())
    {
        ok = _state->exportRaw(filename, whatToExport);
    }
    else if (ui->radioButton_prg->isChecked())
    {
        quint16 addresses[3];
        addresses[0] = ui->spinBox_charsetAddress->value();
        addresses[1] = ui->spinBox_mapAddress->value();
        addresses[2] = ui->spinBox_attribAddress->value();

        ok = _state->exportPRG(filename, addresses, whatToExport);
    }
    else if (ui->radioButton_asm->isChecked())
    {
        ok = _state->exportAsm(filename, whatToExport);
    }

    MainWindow *mainWindow = static_cast<MainWindow*>(parent());

    if (ok) {
        QFileInfo info(filename);
        auto dir = info.absolutePath();
        _settings.setValue(QLatin1String("dir/lastdir"), dir);
        mainWindow->statusBar()->showMessage(tr("File exported to %1").arg(_state->getExportedFilename()), 2000);
    }
    else
    {
        mainWindow->statusBar()->showMessage(tr("Export failed"), 2000);
        qDebug() << "Error saving file: " << filename;
    }

    // do something
    QDialog::accept();
}

void ExportDialog::on_radioButton_raw_clicked()
{
    auto filename = ui->editFilename->text();

    QFileInfo finfo(filename);
    auto extension = finfo.suffix();

    filename.chop(extension.length()+1);
    filename += ".bin";
    ui->editFilename->setText(filename);
}

void ExportDialog::on_radioButton_asm_clicked()
{
    auto filename = ui->editFilename->text();

    QFileInfo finfo(filename);
    auto extension = finfo.suffix();

    filename.chop(extension.length()+1);
    filename += ".s";
    ui->editFilename->setText(filename);
}

void ExportDialog::on_radioButton_prg_clicked()
{
    auto filename = ui->editFilename->text();

    QFileInfo finfo(filename);
    auto extension = finfo.suffix();

    filename.chop(extension.length()+1);
    filename += ".prg";
    ui->editFilename->setText(filename);
}

void ExportDialog::on_checkBox_charset_clicked(bool checked)
{
    if (checked)
        _checkBox_clicked |= State::EXPORT_FEATURE_CHARSET;
    else
        _checkBox_clicked &= ~State::EXPORT_FEATURE_CHARSET;
    updateButtons();
}

void ExportDialog::on_checkBox_map_clicked(bool checked)
{
    if (checked)
        _checkBox_clicked |= State::EXPORT_FEATURE_MAP;
    else
        _checkBox_clicked &= ~State::EXPORT_FEATURE_MAP;
    updateButtons();
}

void ExportDialog::on_checkBox_attribs_clicked(bool checked)
{
    if (checked)
        _checkBox_clicked |= State::EXPORT_FEATURE_ATTRIBS;
    else
        _checkBox_clicked &= ~State::EXPORT_FEATURE_ATTRIBS;
    updateButtons();
}

void ExportDialog::updateButtons()
{
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(_checkBox_clicked != 0);
}
