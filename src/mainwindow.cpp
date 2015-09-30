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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <QDesktopServices>
#include <QCloseEvent>
#include <QUndoView>
#include <QErrorMessage>

#include "state.h"
#include "preview.h"
#include "aboutdialog.h"
#include "exportdialog.h"
#include "tilepropertiesdialog.h"
#include "bigchar.h"
#include "commands.h"
#include "palette.h"

constexpr int MainWindow::MAX_RECENT_FILES;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , _ui(new Ui::MainWindow)
    , _lastDir(QDir::homePath())
    , _settings("RetroMoe","VChar64")
{
    setUnifiedTitleAndToolBarOnMac(true);

    _ui->setupUi(this);

    createActions();
    createDefaults();
    createUndoView();

    readSettings();
}

MainWindow::~MainWindow()
{
    delete _ui;
}

//
// public slots
//
void MainWindow::previewConnected()
{
    _ui->actionXlinkConnection->setText("Disconnect");
}

void MainWindow::previewDisconnected()
{
    _ui->actionXlinkConnection->setText("Connect");
}

void MainWindow::documentWasModified()
{
    auto state = State::getInstance();
    setWindowModified(state->isModified());
}

void MainWindow::updateWindow()
{
    // make sure the "multicolor" checkbox is in the correct state.
    // this is needed for the "undo" / "redos"...
    auto state = State::getInstance();
    _ui->checkBox_multicolor->setChecked(state->isMultiColor());

    update();
}

//
//
//
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();

        saveSettings();
        QMainWindow::closeEvent(event);
    } else {
        event->ignore();
    }
}

void MainWindow::createUndoView()
{
    auto undoDock = new QDockWidget(tr("Undo List"), this);

    auto state = State::getInstance();
    auto undoView = new QUndoView(state->getUndoStack(), undoDock);

    undoDock->setWidget(undoView);
    undoDock->setFloating(true);
    undoDock->hide();

    _ui->menuViews->addAction(undoDock->toggleViewAction());
}

void MainWindow::readSettings()
{
    auto geom = _settings.value("MainWindow/geometry").toByteArray();
    auto state = _settings.value("MainWindow/windowState").toByteArray();

    restoreState(state);
    restoreGeometry(geom);

    QAction* actions[] = {
        _ui->actionPalette_0,
        _ui->actionPalette_1,
        _ui->actionPalette_2,
    };
    int index = _settings.value("palette").toInt();
    actions[index]->trigger();
}

void MainWindow::saveSettings()
{
    _settings.setValue("MainWindow/geometry", saveGeometry());
    _settings.setValue("MainWindow/windowState", saveState());
    _settings.setValue("palette", Palette::getActivePaletteIndex());
}

void MainWindow::createActions()
{
    auto state = State::getInstance();
    connect(state, &State::tilePropertiesUpdated, _ui->bigchar, &BigChar::updateTileProperties);

    // Add recent file actions to the recent files menu
    for (int i=0; i<MAX_RECENT_FILES; ++i)
    {
         _recentFiles[i] = new QAction(this);
         _ui->menuRecentFiles->insertAction(_ui->actionClearRecentFiles, _recentFiles[i]);
         _recentFiles[i]->setVisible(false);
         connect(_recentFiles[i], &QAction::triggered, this, &MainWindow::on_openRecentFile_triggered);
    }
    _ui->menuRecentFiles->insertSeparator(_ui->actionClearRecentFiles);
    updateRecentFiles();


    // FIXME should be on a different method
    _ui->colorRect_0->setColorIndex(0);
    _ui->colorRect_1->setColorIndex(3);
    _ui->colorRect_2->setColorIndex(1);
    _ui->colorRect_3->setColorIndex(2);

    auto preview = Preview::getInstance();
    connect(state, SIGNAL(fileLoaded()), preview, SLOT(fileLoaded()));
    connect(state, SIGNAL(byteUpdated(int)), preview, SLOT(byteUpdated(int)));
    connect(state, SIGNAL(tileUpdated(int)), preview, SLOT(tileUpdated(int)));
    connect(state, SIGNAL(byteUpdated(int)), this, SLOT(updateWindow()));
    connect(state, SIGNAL(tileUpdated(int)), this, SLOT(updateWindow()));
//    connect(state, SIGNAL(colorPropertiesUpdated()), preview, SLOT(tileWasUpdated()));
    connect(state, SIGNAL(colorPropertiesUpdated()), this, SLOT(updateWindow()));
    connect(state, SIGNAL(contentsChanged()), this, SLOT(documentWasModified()));
    connect(state->getUndoStack(), SIGNAL(indexChanged(int)), this, SLOT(documentWasModified()));
    connect(state->getUndoStack(), SIGNAL(cleanChanged(bool)), this, SLOT(documentWasModified()));

    connect(_ui->paletteView, SIGNAL(colorSelected()), preview, SLOT(colorSelected()));
    connect(preview, SIGNAL(previewConnected()), this, SLOT(previewConnected()));
    connect(preview, SIGNAL(previewDisconnected()), this, SLOT(previewDisconnected()));

    _ui->menuPreview->setEnabled(preview->isAvailable());

//
    _ui->menuViews->addAction(_ui->dockWidget_charset->toggleViewAction());
    _ui->menuViews->addAction(_ui->dockWidget_colors->toggleViewAction());
    _ui->menuViews->addAction(_ui->dockWidget_tileIndex->toggleViewAction());


     if(preview->isConnected()) {
          previewConnected();
    }
}

void MainWindow::createDefaults()
{
    _lastDir = _settings.value("dir/lastdir", _lastDir).toString();

    auto state = State::getInstance();
    state->openFile(":/c64-chargen-uppercase.bin");

    State::TileProperties properties;
    properties.size = {1,1};
    properties.interleaved = 1;
    state->setTileProperties(properties);

    state->setMultiColor(false);

    setWindowFilePath("[untitled]");
}

QStringList MainWindow::recentFiles() const
{
    QVariant v = _settings.value(QLatin1String("recentFiles/fileNames"));
    return v.toStringList();
}

void MainWindow::updateRecentFiles()
{
    QStringList files = recentFiles();
    const int numRecentFiles = qMin(files.size(), MAX_RECENT_FILES);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        _recentFiles[i]->setText(QFileInfo(files[i]).fileName());
        _recentFiles[i]->setData(files[i]);
        _recentFiles[i]->setVisible(true);
    }
    for (int j=numRecentFiles; j<MAX_RECENT_FILES; ++j)
    {
        _recentFiles[j]->setVisible(false);
    }
    _ui->menuRecentFiles->setEnabled(numRecentFiles > 0);
}

void MainWindow::setRecentFile(const QString& fileName)
{
    // Remember the file by its canonical file path
    const QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    if (canonicalFilePath.isEmpty())
        return;

    QStringList files = recentFiles();
    files.removeAll(canonicalFilePath);
    files.prepend(canonicalFilePath);
    while (files.size() > MAX_RECENT_FILES)
        files.removeLast();

    _settings.setValue(QLatin1String("recentFiles/fileNames"), files);
    updateRecentFiles();
}

//
// MARK - Slots / Events / Callbacks
//
void MainWindow::on_actionExit_triggered()
{
    if (maybeSave()) {
        saveSettings();
        QApplication::exit();
    }
}

void MainWindow::on_actionEmptyProject_triggered()
{
    if (maybeSave())
    {
        auto state = State::getInstance();
        state->reset();
        update();
        setWindowFilePath("[untitled]");
    }
}

void MainWindow::on_actionC64DefaultUppercase_triggered()
{
    if (maybeSave())
    {
        auto state = State::getInstance();
        state->openFile(":/c64-chargen-uppercase.bin");
        State::TileProperties properties;
        properties.size = {1,1};
        properties.interleaved = 1;
        state->setTileProperties(properties);
        state->setMultiColor(false);

        update();
        setWindowFilePath("[untitled]");
    }
}

void MainWindow::on_actionC64DefaultLowercase_triggered()
{
    if (maybeSave())
    {
        auto state = State::getInstance();
        state->openFile(":/c64-chargen-lowercase.bin");
        State::TileProperties properties;
        properties.size = {1,1};
        properties.interleaved = 1;
        state->setTileProperties(properties);
        state->setMultiColor(false);

        update();
        setWindowFilePath("[untitled]");
    }
}

//
// MARK - Color + Palette callbacks
//
void MainWindow::on_checkBox_multicolor_toggled(bool checked)
{
    _ui->radioButton_multicolor1->setEnabled(checked);
    _ui->radioButton_multicolor2->setEnabled(checked);

    _ui->actionMulti_Color_1->setEnabled(checked);
    _ui->actionMulti_Color_2->setEnabled(checked);

    _ui->actionEnable_Multicolor->setChecked(checked);

    State *state = State::getInstance();

    state->getUndoStack()->push(new SetMulticolorModeCommand(state, checked));
}

void MainWindow::activateRadioButtonIndex(int index)
{
    State *state = State::getInstance();
    state->setSelectedColorIndex(index);

    QAction* actions[] = {
        _ui->actionBackground,
        _ui->actionMulti_Color_1,
        _ui->actionMulti_Color_2,
        _ui->actionForeground
    };

    for (int i=0; i<4; i++)
        actions[i]->setChecked(i==index);
}

void MainWindow::on_radioButton_background_clicked()
{
    activateRadioButtonIndex(0);
}

void MainWindow::on_radioButton_foreground_clicked()
{
    activateRadioButtonIndex(3);
}

void MainWindow::on_radioButton_multicolor1_clicked()
{
    activateRadioButtonIndex(1);
}

void MainWindow::on_radioButton_multicolor2_clicked()
{
    activateRadioButtonIndex(2);
}

void MainWindow::on_actionBackground_triggered()
{
    _ui->radioButton_background->click();
}

void MainWindow::on_actionForeground_triggered()
{
    _ui->radioButton_foreground->click();
}

void MainWindow::on_actionMulti_Color_1_triggered()
{
    _ui->radioButton_multicolor1->click();
}

void MainWindow::on_actionMulti_Color_2_triggered()
{
    _ui->radioButton_multicolor2->click();
}

void MainWindow::on_actionEnable_Multicolor_triggered()
{
    _ui->checkBox_multicolor->click();
}

void MainWindow::on_actionPalette_0_triggered()
{
    Palette::setActivePaletteIndex(0);
    _ui->actionPalette_1->setChecked(false);
    _ui->actionPalette_2->setChecked(false);
    update();
}

void MainWindow::on_actionPalette_1_triggered()
{
    Palette::setActivePaletteIndex(1);
    _ui->actionPalette_0->setChecked(false);
    _ui->actionPalette_2->setChecked(false);
    update();
}

void MainWindow::on_actionPalette_2_triggered()
{
    Palette::setActivePaletteIndex(2);
    _ui->actionPalette_0->setChecked(false);
    _ui->actionPalette_1->setChecked(false);
    update();
}

//
// MARK - File IO callbacks + helper functions
//
void MainWindow::openFile(const QString& fileName)
{
    QFileInfo info(fileName);
    _lastDir = info.absolutePath();
    _settings.setValue("dir/lastdir", _lastDir);

    if (State::getInstance()->openFile(fileName)) {

        setRecentFile(fileName);

        update();
        auto state = State::getInstance();
        _ui->checkBox_multicolor->setChecked(state->isMultiColor());

        setWindowFilePath(info.filePath());
    }
    else
    {
        QMessageBox::warning(this, tr("Application"), tr("Error loading file: ") + fileName, QMessageBox::Ok);
    }
}

bool MainWindow::maybeSave()
{
    auto state = State::getInstance();

    if (state->isModified()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Application"),
                     tr("The are unsaved changes.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return on_actionSave_triggered();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void MainWindow::on_actionOpen_triggered()
{
    if (maybeSave())
    {
        QString filter = _settings.value("dir/lastUsedOpenFilter", "All supported files").toString();
        auto fn = QFileDialog::getOpenFileName(this,
                                               tr("Select File"),
                                               _lastDir,
                                               tr(
                                                   "All files (*);;" \
                                                   "All supported files (*.vchar64proj *.raw *.bin *.prg *.64c *.ctm);;" \
                                                   "VChar64 Project (*.vchar64proj);;" \
                                                   "Raw (*.raw *.bin);;" \
                                                   "PRG (*.prg *.64c);;" \
                                                   "CharPad (*.ctm);;"
                                               ),
                                               &filter
                                               /*,QFileDialog::DontUseNativeDialog*/
                                               );

        if (fn.length()> 0) {
            _settings.setValue("dir/lastUsedOpenFilter", filter);
            openFile(fn);
        }
    }
}

bool MainWindow::on_actionSaveAs_triggered()
{
    bool ret = false;

    auto state = State::getInstance();
    auto fn = state->getSavedFilename();
    if (fn.length() == 0)
        fn = state->getLoadedFilename();
    if (fn.length() != 0)
    {
        QFileInfo fileInfo(fn);
        auto suffix = fileInfo.suffix();
        if (suffix != "vchar64proj") {
            fn = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".vchar64proj";
        }
    }
    else
    {
        fn = _lastDir + "/untitled.vchar64proj";
    }
    auto filename = QFileDialog::getSaveFileName(this, tr("Save Project"),
                                             fn,
                                             tr("VChar64 project(*.vchar64proj)"));

    if (filename.length() > 0) {
        auto state = State::getInstance();
        if ((ret=state->saveProject(filename))) {
            QFileInfo fi(filename);
            setWindowFilePath(fi.filePath());
        }
    }

    if (!ret)
        QMessageBox::warning(this, tr("Application"), tr("Error saving project file: ") + filename, QMessageBox::Ok);

    return ret;
}

bool MainWindow::on_actionSave_triggered()
{
    bool ret;
    auto state = State::getInstance();
    auto filename = state->getSavedFilename();
    if (filename.length() > 0)
        ret = state->saveProject(filename);
    ret = on_actionSaveAs_triggered();

    if (!ret)
        QMessageBox::warning(this, tr("Application"), tr("Error saving project file: ") + filename, QMessageBox::Ok);

    return ret;
}

void MainWindow::on_actionExport_triggered()
{
    auto state = State::getInstance();
    auto exportedFilename = state->getExportedFilename();
    if (exportedFilename.length()==0)
    {
        ExportDialog dialog(this);
        dialog.exec();
    }
    else
    {
        if (!state->export_())
        {
            QMessageBox::warning(this, tr("Application"), tr("Error exporting file: ") + exportedFilename, QMessageBox::Ok);
        }
    }
}

void MainWindow::on_actionExportAs_triggered()
{
    ExportDialog dialog(this);
    dialog.exec();
}

//
// MARK - Tile editing callbacks
//
void MainWindow::on_actionInvert_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new InvertTileCommand(state, tileIndex));
}

void MainWindow::on_actionFlipHorizontally_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new FlipTileHCommand(state, tileIndex));
}

void MainWindow::on_actionFlipVertically_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new FlipTileVCommand(state, tileIndex));
}

void MainWindow::on_actionRotate_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new RotateTileCommand(state, tileIndex));
}

void MainWindow::on_actionClearCharacter_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new ClearTileCommand(state, tileIndex));
}

void MainWindow::on_actionShiftLeft_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new ShiftLeftTileCommand(state, tileIndex));
}

void MainWindow::on_actionShiftRight_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new ShiftRightTileCommand(state, tileIndex));
}

void MainWindow::on_actionShiftUp_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new ShiftUpTileCommand(state, tileIndex));
}

void MainWindow::on_actionShiftDown_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new ShiftDownTileCommand(state, tileIndex));
}

void MainWindow::on_actionCopy_triggered()
{
    auto state = State::getInstance();
    state->tileCopy(_ui->bigchar->getTileIndex());
}

void MainWindow::on_actionPaste_triggered()
{
    auto state = State::getInstance();
    int tileIndex = _ui->bigchar->getTileIndex();

    state->getUndoStack()->push(new PasteTileCommand(state, tileIndex));
}

void MainWindow::on_actionReportBug_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/ricardoquesada/vchar64/issues"));
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}

void MainWindow::on_actionAboutQt_triggered()
{
    QApplication::aboutQt();
}

void MainWindow::on_actionClearRecentFiles_triggered()
{
    _settings.setValue(QLatin1String("recentFiles/fileNames"), QStringList());
    updateRecentFiles();
}

void MainWindow::on_openRecentFile_triggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        openFile(action->data().toString());
}

void MainWindow::on_actionTilesProperties_triggered()
{
    TilePropertiesDialog dialog(this);

    dialog.exec();

    // update max tile index
    auto state = State::getInstance();
    QSize s = state->getTileProperties().size;
    _ui->spinBox_tileIndex->setMaximum((256 / (s.width()*s.height()))-1);

    // rotate only enable if witdh==height
    _ui->actionRotate->setEnabled(s.width() == s.height());
}

void MainWindow::on_actionXlinkConnection_triggered()
{
    auto preview = Preview::getInstance();
    if(preview->isConnected())
        preview->disconnect();
    else
        if(!preview->connect()) {
            QMessageBox msgBox(QMessageBox::Warning, "", "Could not connect to remote C64", 0, this);
            msgBox.exec();
        }
}

void MainWindow::on_actionUndo_triggered()
{
    auto state = State::getInstance();
    state->getUndoStack()->undo();
}

void MainWindow::on_actionRedo_triggered()
{
    auto state = State::getInstance();
    state->getUndoStack()->redo();
}
