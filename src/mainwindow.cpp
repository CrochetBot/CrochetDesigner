/*************************************************\
| Copyright (c) 2010 Stitch Works Software        |
| Brian C. Milco <brian@stitchworkssoftware.com>  |
\*************************************************/
#include "mainwindow.h"
#include "src/ui_mainwindow.h"
#include "stitchlibraryui.h"
#include "licensewizard.h"
#include "exportui.h"

#include "appinfo.h"
#include "settings.h"
#include "settingsui.h"

#include "crochettab.h"
#include "stitchlibrary.h"
#include "stitchset.h"
#include "stitchpalettedelegate.h"

#include <QDebug>
#include <QDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QColorDialog>

#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>

#include <QActionGroup>
#include <QCloseEvent>
#include <QUndoStack>
#include <QUndoView>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent, QString fileName)
    : QMainWindow(parent), ui(new Ui::MainWindow), mEditMode(10), mStitch("ch"),
    mFgColor(QColor(Qt::black)), mBgColor(QColor(Qt::white))
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    ui->setupUi(this);

    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(removeTab(int)));
    
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    checkUpdates();
    setupStitchPalette();
    setupUndoView();

    mFile = new SaveFile(this);
    if(!fileName.isEmpty()) {
        mFile->fileName = fileName;
        mFile->load(); //TODO: if not error then hide the dialog.
        ui->newDocument->hide();
    } else {
        openCommandLineFiles();
    }

    setApplicationTitle();
    setupNewTabDialog();

    setupMenus();
    readSettings();
    QApplication::restoreOverrideCursor();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openCommandLineFiles()
{
    //FIXME: make sure this works on a mac!
    QStringList arguments = QCoreApplication::arguments();
    arguments.removeFirst(); // remove the application name from the list.
  
    if(arguments.count() < 1)
        return;

    if(ui->tabWidget->count() < 1) {
        mFile->fileName = arguments.takeFirst();
        mFile->load(); //TODO: if !error hide dialog.
        ui->newDocument->hide();
    }

    foreach(QString fileName, arguments) {
        MainWindow *newWin = new MainWindow(0, fileName);
        newWin->move(x() + 40, y() + 40);
        newWin->show();
    }
}

void MainWindow::checkUpdates()
{
    //TODO: check for updates in a seperate thread.
    mUpdater = new Updater(this);
    // append the updater to the centralWidget to keep it out of the way of the menus.
    ui->centralWidget->layout()->addWidget(mUpdater); 
        
    bool checkForUpdates = Settings::inst()->value("checkForUpdates").toBool();
    if(checkForUpdates)
        mUpdater->checkForUpdates(true); //check at startup is always silent.
}

void MainWindow::setApplicationTitle()
{
    QString cleanName = QFileInfo(mFile->fileName).baseName();
    if(cleanName.isEmpty())
        cleanName = "untitled";
    
    setWindowTitle(QString("%1 - %2[*]").arg(qApp->applicationName()).arg(cleanName));
}

void MainWindow::setupNewTabDialog()
{
    int rows = Settings::inst()->value("rowCount").toInt();
    int stitches = Settings::inst()->value("stitchCount").toInt();
    QString defSt = Settings::inst()->value("defaultStitch").toString();
    QString defStyle = Settings::inst()->value("chartStyle").toString();
    
    ui->rows->setValue(rows);
    ui->stitches->setValue(stitches);

    ui->defaultStitch->addItems(StitchLibrary::inst()->stitchList());
    ui->defaultStitch->setCurrentIndex(ui->defaultStitch->findText(defSt));

    ui->chartStyle->setCurrentIndex(ui->chartStyle->findText(defStyle));
    
    //TODO: see if you can make "returnPressed" focus and click the ok button for the spin boxes.
    connect(ui->chartTitle, SIGNAL(returnPressed()), this, SLOT(newChart()));
    
    connect(ui->newDocBttnBox, SIGNAL(accepted()), this, SLOT(newChart()));
    connect(ui->newDocBttnBox, SIGNAL(rejected()), ui->newDocument, SLOT(hide()));   
}

void MainWindow::setupStitchPalette()
{
    StitchSet *set = StitchLibrary::inst()->masterStitchSet();
    ui->allStitches->setModel(set);

    //TODO: setup a proxywidget that can hold header sections?
    //StitchPaletteDelegate *delegate = new StitchPaletteDelegate(ui->allStitches);
    //ui->allStitches->setItemDelegate(delegate);

    connect(ui->allStitches, SIGNAL(clicked(QModelIndex)), this, SLOT(selectStitch(QModelIndex)));
    connect(ui->patternStitches, SIGNAL(clicked(QModelIndex)), this, SLOT(selectStitch(QModelIndex)));
    connect(ui->patternColors, SIGNAL(clicked(QModelIndex)), this, SLOT(selectColor(QModelIndex)));
}

void MainWindow::setupUndoView()
{
    mUndoDock = new QDockWidget(this);
    mUndoDock->setObjectName("undoHistory");
    QUndoView *view = new QUndoView(&mUndoGroup, mUndoDock);
    mUndoDock->setWidget(view);
    mUndoDock->setWindowTitle(tr("Undo History"));
    mUndoDock->setFloating(true);
    mUndoDock->setVisible(false);
}

void MainWindow::setupMenus()
{
    //File Menu
    connect(ui->menuFile, SIGNAL(aboutToShow()), this, SLOT(menuFileAboutToShow()));
    connect(ui->actionNew, SIGNAL(triggered()), this, SLOT(fileNew()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(fileOpen()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(fileSave()));
    connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(fileSaveAs()));

    connect(ui->actionPrint, SIGNAL(triggered()), this, SLOT(filePrint()));
    connect(ui->actionPrintPreview, SIGNAL(triggered()), this, SLOT(filePrintPreview()));
    connect(ui->actionExport, SIGNAL(triggered()), this, SLOT(fileExport()));

    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
  
    ui->actionOpen->setIcon(QIcon::fromTheme("document-open" /*, QIcon(":/file-open.png")*/));
    ui->actionNew->setIcon(QIcon::fromTheme("document-new"));
    ui->actionSave->setIcon(QIcon::fromTheme("document-save"));
    ui->actionSaveAs->setIcon(QIcon::fromTheme("document-save-as"));

    ui->actionPrint->setIcon(QIcon::fromTheme("document-print"));
    ui->actionPrintPreview->setIcon(QIcon::fromTheme("document-print-preview"));
    ui->actionExport->setIcon(QIcon::fromTheme("document-export"));
    
    ui->actionQuit->setIcon(QIcon::fromTheme("application-exit"));

    //Edit Menu
    connect(ui->menuEdit, SIGNAL(aboutToShow()), this, SLOT(menuEditAboutToShow()));

    mActionUndo = mUndoGroup.createUndoAction(this, tr("Undo"));
    mActionRedo = mUndoGroup.createRedoAction(this, tr("Redo"));
    
    ui->menuEdit->insertAction(ui->actionCopy, mActionUndo);
    ui->menuEdit->insertAction(ui->actionCopy, mActionRedo);
    ui->menuEdit->insertSeparator(ui->actionCopy);

    ui->mainToolBar->insertAction(0, mActionUndo);
    ui->mainToolBar->insertAction(0, mActionRedo);
    ui->mainToolBar->insertSeparator(mActionUndo);
    
    mActionUndo->setIcon(QIcon::fromTheme("edit-undo"));
    mActionRedo->setIcon(QIcon::fromTheme("edit-redo"));
    mActionUndo->setShortcut(QKeySequence::Undo);
    mActionRedo->setShortcut(QKeySequence::Redo);
    
    ui->actionCopy->setIcon(QIcon::fromTheme("edit-copy" /*, QIcon(":/edit-copy.png")*/));
    ui->actionCut->setIcon(QIcon::fromTheme("edit-cut" /*, QIcon(":/edit-cut.png")*/));
    ui->actionPaste->setIcon(QIcon::fromTheme("edit-paste" /*, QIcon(":/edit-paste.png")*/));


    connect(ui->actionColorSelectorBg, SIGNAL(triggered()), this, SLOT(selectColor()));
    connect(ui->actionColorSelectorFg, SIGNAL(triggered()), this, SLOT(selectColor()));
    
    ui->actionColorSelectorFg->setIcon(QIcon(drawColorBox(mFgColor, QSize(32, 32))));
    ui->actionColorSelectorBg->setIcon(QIcon(drawColorBox(mBgColor, QSize(32, 32))));
    
    //View Menu
    connect(ui->menuView, SIGNAL(aboutToShow()), this, SLOT(menuViewAboutToShow()));
    connect(ui->actionShowStitches, SIGNAL(triggered()), this, SLOT(viewShowStitches()));
    connect(ui->actionShowPatternColors, SIGNAL(triggered()), this, SLOT(viewShowPatternColors()));
    connect(ui->actionShowPatternStitches, SIGNAL(triggered()), this, SLOT(viewShowPatternStitches()));

    connect(ui->actionShowUndoHistory, SIGNAL(triggered()), this, SLOT(viewShowUndoHistory()));
    
    connect(ui->actionShowMainToolbar, SIGNAL(triggered()), this, SLOT(viewShowMainToolbar()));
    connect(ui->actionShowEditModeToolbar, SIGNAL(triggered()), this, SLOT(viewShowEditModeToolbar()));
    
    connect(ui->actionViewFullScreen, SIGNAL(triggered(bool)), this, SLOT(viewFullScreen(bool)));

    connect(ui->actionZoomIn, SIGNAL(triggered()), this, SLOT(viewZoomIn()));
    connect(ui->actionZoomOut, SIGNAL(triggered()), this, SLOT(viewZoomOut()));
    
    ui->actionZoomIn->setIcon(QIcon::fromTheme("zoom-in"));
    ui->actionZoomOut->setIcon(QIcon::fromTheme("zoom-out"));
    ui->actionZoomIn->setShortcut(QKeySequence::ZoomIn);
    ui->actionZoomOut->setShortcut(QKeySequence::ZoomOut);

    //Modes menu
    connect(ui->menuModes, SIGNAL(aboutToShow()), this, SLOT(menuModesAboutToShow()));

    mModeGroup = new QActionGroup(this);
    mModeGroup->addAction(ui->actionStitchMode);
    mModeGroup->addAction(ui->actionColorMode);
    mModeGroup->addAction(ui->actionGridMode);
    mModeGroup->addAction(ui->actionPositionMode);
    mModeGroup->addAction(ui->actionAngleMode);
    mModeGroup->addAction(ui->actionStrechMode);
    
    ui->actionColorMode->setIcon(QIcon::fromTheme("fill-color"));

    connect(mModeGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeTabMode(QAction*)));
    
    //Document Menu
    connect(ui->menuDocument, SIGNAL(aboutToShow()), this, SLOT(menuDocumentAboutToShow()));
    connect(ui->actionAddChart, SIGNAL(triggered()), this, SLOT(documentNewChart()));

    connect(ui->actionRemoveTab, SIGNAL(triggered()), this, SLOT(removeCurrentTab()));

    ui->actionAddChart->setIcon(QIcon::fromTheme("tab-new")); //insert-chart
    ui->actionRemoveTab->setIcon(QIcon::fromTheme("tab-close"));

    //Chart Menu
    connect(ui->menuChart, SIGNAL(aboutToShow()), this, SLOT(menuChartAboutToShow()));
    connect(ui->actionEditName, SIGNAL(triggered()), this, SLOT(chartEditName()));
    //TODO: get more icons from the theme for use with table editing.
    //http://doc.qt.nokia.com/4.7/qstyle.html#StandardPixmap-enum
    
    //Tools Menu
    connect(ui->actionOptions, SIGNAL(triggered()), this, SLOT(toolsOptions()));
    connect(ui->actionRegisterSoftware, SIGNAL(triggered()), this, SLOT(toolsRegisterSoftware()));
    connect(ui->actionStitchLibrary, SIGNAL(triggered()), this, SLOT(toolsStitchLibrary()));
    connect(ui->actionCheckForUpdates, SIGNAL(triggered()), this, SLOT(toolsCheckForUpdates()));
    
    if(!Settings::inst()->isDemoVersion())
        ui->actionRegisterSoftware->setVisible(false);

    //Help Menu
    connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(helpAbout()));
    
    //misc items
    connect(&mUndoGroup, SIGNAL(documentCleanChanged(bool)), this, SLOT(documentIsModified(bool)));
    
    updateMenuItems();
}

void MainWindow::updateMenuItems()
{
    menuFileAboutToShow();
    menuEditAboutToShow();
    menuViewAboutToShow();
    menuModesAboutToShow();
    menuDocumentAboutToShow();
    menuChartAboutToShow();
}

void MainWindow::filePrint()
{
    //TODO: page count isn't working...
    QPrinter *printer = new QPrinter();
    QPrintDialog *dialog = new QPrintDialog(printer, this);

    if(dialog->exec() != QDialog::Accepted)
        return;

    print(printer);
}

void MainWindow::print(QPrinter *printer)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    
    int tabCount = ui->tabWidget->count();
    QPainter *p = new QPainter();
    
    p->begin(printer);
    
    bool firstPass = true;
    for(int i = 0; i < tabCount; ++i) {
        if(!firstPass)
            printer->newPage();
        
        CrochetTab *tab = qobject_cast<CrochetTab*>(ui->tabWidget->widget(i));
        tab->renderChart(p);
        firstPass = false;
    }
    p->end();
    
    QApplication::restoreOverrideCursor();
}

void MainWindow::filePrintPreview()
{
    //FIXME: this isn't working
    QPrinter *printer = new QPrinter(QPrinter::HighResolution);
    QPrintPreviewDialog *dialog = new QPrintPreviewDialog(printer, this);
    connect(dialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(print(QPrinter*)));
    
    dialog->exec();
}

void MainWindow::fileExport()
{
    if(!hasTab())
        return;
    
    ExportUi d(ui->tabWidget, &mPatternStitches, &mPatternColors, this);
    d.exec();
}

QPixmap MainWindow::drawColorBox(QColor color, QSize size)
{
    QPixmap pix = QPixmap(size);
    QPainter p;
    p.begin(&pix);
    p.fillRect(QRect(QPoint(0, 0), size), QColor(color));
    p.drawRect(0, 0, size.width() - 1, size.height() - 1);
    p.end();

    return pix;
}

void MainWindow::selectColor()
{
    if(sender() == ui->actionColorSelectorBg) {
        QColor color = QColorDialog::getColor(mBgColor, this, tr("Background Color"));
        
        if (color.isValid()) {
            ui->actionColorSelectorBg->setIcon(QIcon(drawColorBox(QColor(color), QSize(32, 32))));
            mBgColor = color;
            updateBgColor();
        }
    } else {
        QColor color = QColorDialog::getColor(mFgColor, this, tr("Foreground Color"));
        
        if (color.isValid()) {
            ui->actionColorSelectorFg->setIcon(QIcon(drawColorBox(QColor(color), QSize(32, 32))));
            mFgColor = color;
            updateFgColor();
        }
    }
}

void MainWindow::updateBgColor()
{
    for(int i = 0; i < ui->tabWidget->count(); ++i) {
        CrochetTab *tab = qobject_cast<CrochetTab*>(ui->tabWidget->widget(i));
        if(tab)
            tab->setEditBgColor(mBgColor);
    }
    setEditMode(11);
}

void MainWindow::updateFgColor()
{
    for(int i = 0; i < ui->tabWidget->count(); ++i) {
        CrochetTab *tab = qobject_cast<CrochetTab*>(ui->tabWidget->widget(i));
        if(tab)
            tab->setEditFgColor(mFgColor);
    }
    setEditMode(11);
}

void MainWindow::selectStitch(QModelIndex index)
{
    QString stitch = index.data(Qt::DisplayRole).toString();

    if(stitch.isEmpty())
        return;
    
    for(int i = 0; i < ui->tabWidget->count(); ++i) {
        CrochetTab *tab = qobject_cast<CrochetTab*>(ui->tabWidget->widget(i));
        if(tab)
            tab->setEditStitch(stitch);
    }
    setEditMode(10);
}

void MainWindow::selectColor(QModelIndex index)
{
    QString color = index.data(Qt::ToolTipRole).toString();

    for(int i = 0; i < ui->tabWidget->count(); ++i) {
        CrochetTab *tab = qobject_cast<CrochetTab*>(ui->tabWidget->widget(i));
        if(tab)
            tab->setEditBgColor(color);
    }
    setEditMode(11);
}

void MainWindow::documentNewChart()
{
    int rows = Settings::inst()->value("rowCount").toInt();
    int stitches = Settings::inst()->value("stitchCount").toInt();
    
    ui->rows->setValue(rows);
    ui->stitches->setValue(stitches);
    
    ui->chartTitle->setText(nextChartName());

    if(ui->newDocument->isVisible()) {
        QPalette pal = ui->newDocument->palette();
        mNewDocWidgetColor = ui->newDocument->palette().color(ui->newDocument->backgroundRole());
        pal.setColor(ui->newDocument->backgroundRole(), ui->newDocument->palette().highlight().color());
        ui->newDocument->setPalette(pal);
        QTimer::singleShot(1500, this, SLOT(flashNewDocDialog()));
    } else 
        ui->newDocument->show();
}

void MainWindow::helpAbout()
{
    QString aboutInfo = QString(tr("<h1>%1</h1>"
                                   "<p>Version: %2 (built on %3)</p>"
                                   "<p>Copyright (c) 2010-2011 %4</p>"
                                   "<p>This software is for creating crochet charts that"
                                   " can be exported in many differnet file types.</p>")
                                .arg(qApp->applicationName())
                                .arg(qApp->applicationVersion())
                                .arg(AppInfo::appBuildInfo)
                                .arg(qApp->organizationName())
                                );
    QString fName = Settings::inst()->value("firstName").toString();
    QString lName = Settings::inst()->value("lastName").toString();
    QString email = Settings::inst()->value("email").toString();
    QString sn = Settings::inst()->value("serialNumber").toString();

    QString dedication = tr("<p>This version is dedicated to my Nana (Apr 28, 1927 - Feb 21, 2011)</p>");
    aboutInfo.append(dedication);
    
    QString licenseInfo;

    if(Settings::inst()->isDemoVersion()) {
        licenseInfo = QString(tr("<p>This is a demo version licensed to:<br />"
                              "Name: %1 %2<br />"
                              "Email: %3<br /></p>")
                              .arg(fName).arg(lName).arg(email));
    } else {
        licenseInfo = QString(tr("<p>This software is licensed to:<br />"
                              "Name: %1 %2<br />"
                              "Email: %3<br />"
                              "Serial #: %4</p>")
                              .arg(fName).arg(lName).arg(email).arg(sn));
    }

    aboutInfo.append(licenseInfo);
    QMessageBox::about(this, tr("About Crochet"), aboutInfo);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(safeToClose()) {
        Settings::inst()->setValue("geometry", saveGeometry());
        Settings::inst()->setValue("windowState", saveState());
        QMainWindow::closeEvent(event);
    } else {
        event->ignore();
    }
}

bool MainWindow::safeToClose()
{
    foreach(QUndoStack *stack, mUndoGroup.stacks()) {
        if(!stack->isClean())
            return promptToSave();
    }

    if(mFile->fileName.isEmpty() && mUndoGroup.stacks().count() > 0)
        return promptToSave();

    return true;
}

bool MainWindow::promptToSave()
{
    QString niceName = QFileInfo(mFile->fileName).baseName();
    if(niceName.isEmpty())
        niceName = "untitled";
    
    QMessageBox msgbox(this);
    msgbox.setText(tr("The document '%1' has unsaved changes.").arg(niceName));
    msgbox.setInformativeText(tr("Do you want to save the changes?"));
    msgbox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    
    int results = msgbox.exec();
    
    if(results == QMessageBox::Cancel)
        return false;
    else if(results == QMessageBox::Discard)
        return true;
    else if (results == QMessageBox::Save) {
        //FIXME: if the user cancels the fileSave() we should drop them back to the window not close it.
        fileSave();
        return true;
    }

    return false;
}

void MainWindow::readSettings()
{
    //TODO: For full session restoration reimplement QApplication::commitData()
    //See: http://doc.qt.nokia.com/stable/session.html
    restoreGeometry(Settings::inst()->value("geometry").toByteArray());
    restoreState(Settings::inst()->value("windowState").toByteArray());

}

void MainWindow::toolsOptions()
{
    SettingsUi dialog(this);
    dialog.exec();
}

void MainWindow::fileOpen()
{
    QString fileLoc = Settings::inst()->value("fileLocation").toString();
    QString fileName = QFileDialog::getOpenFileName(this,
         tr("Open Crochet Pattern"), fileLoc, tr("Crochet Pattern (*.pattern)"));

    if(fileName.isEmpty() || fileName.isNull())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    if(ui->tabWidget->count() > 0) {
        MainWindow *newWin = new MainWindow(0, fileName);
        newWin->move(x() + 40, y() + 40);
        newWin->show();
    } else {
        ui->newDocument->hide();
        mFile->fileName = fileName;
        mFile->load();
    }

    setApplicationTitle();
    updateMenuItems();
    QApplication::restoreOverrideCursor();
}

void MainWindow::fileSave()
{
    if(Settings::inst()->isDemoVersion()) {
        Settings::inst()->trialVersionMessage(this);
        return;
    }

    if(mFile->fileName.isEmpty())
        fileSaveAs();
    else {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        SaveFile::FileError err = mFile->save();
        if(err != SaveFile::No_Error)
            qWarning() << "There was an error saving the file: " << err;
        QApplication::restoreOverrideCursor();
    }
    
    setWindowModified(false);
}

void MainWindow::fileSaveAs()
{
    if(Settings::inst()->isDemoVersion()) {
        Settings::inst()->trialVersionMessage(this);
        return;
    }

    QString fileLoc = Settings::inst()->value("fileLocation").toString();
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Crochet Pattern"), fileLoc, tr("Crochet Pattern (*.pattern)"));

    if(fileName.isEmpty())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    mFile->fileName = fileName;
    mFile->save();

    setApplicationTitle();
    setWindowModified(false);
    QApplication::restoreOverrideCursor();
}

void MainWindow::viewFullScreen(bool state)
{
    if(state)
        showFullScreen();
    else
        showNormal();
}

void MainWindow::menuFileAboutToShow()
{
    bool state = hasTab();

    ui->actionSave->setEnabled(state);
    ui->actionSaveAs->setEnabled(state);

    ui->actionPrint->setEnabled(state);
    ui->actionPrintPreview->setEnabled(state);
    
    ui->actionExport->setEnabled(state);

    ui->actionColorSelectorFg->setEnabled(state);
    ui->actionColorSelectorBg->setEnabled(state);
}

void MainWindow::menuEditAboutToShow()
{
    bool state = hasTab();
    
    ui->actionCopy->setEnabled(state);
    ui->actionCut->setEnabled(state);
    ui->actionPaste->setEnabled(state);
    
}

void MainWindow::menuViewAboutToShow()
{
    ui->actionShowStitches->setChecked(ui->allStitchesDock->isVisible());
    ui->actionShowPatternColors->setChecked(ui->patternColorsDock->isVisible());
    ui->actionShowPatternStitches->setChecked(ui->patternStitchesDock->isVisible());

    ui->actionShowUndoHistory->setChecked(mUndoDock->isVisible());
    
    ui->actionShowEditModeToolbar->setChecked(ui->editModeToolBar->isVisible());
    ui->actionShowMainToolbar->setChecked(ui->mainToolBar->isVisible());
    
    ui->actionViewFullScreen->setChecked(isFullScreen());

    bool state = hasTab();
    ui->actionZoomIn->setEnabled(state);
    ui->actionZoomOut->setEnabled(state);
}

void MainWindow::fileNew()
{

    if(ui->tabWidget->count() > 0) {
        MainWindow *newWin = new MainWindow;
        newWin->move(x() + 40, y() + 40);
        newWin->show();
        newWin->ui->newDocument->show();
    } else {
        if(ui->newDocument->isVisible()) {
            QPalette pal = ui->newDocument->palette();
            mNewDocWidgetColor = ui->newDocument->palette().color(ui->newDocument->backgroundRole());
            pal.setColor(ui->newDocument->backgroundRole(), ui->newDocument->palette().highlight().color());
            ui->newDocument->setPalette(pal);
            QTimer::singleShot(1500, this, SLOT(flashNewDocDialog()));
        } else
            ui->newDocument->show();
    }
    
}

void MainWindow::flashNewDocDialog()
{
    QPalette pal = ui->newDocument->palette();
    pal.setColor(ui->newDocument->backgroundRole(), mNewDocWidgetColor);
    ui->newDocument->setPalette(pal);
}

void MainWindow::newChart()
{
    ui->newDocument->hide();

    int rows = ui->rows->text().toInt();
    int cols = ui->stitches->text().toInt();
    QString defStitch = ui->defaultStitch->currentText();
    QString name = ui->chartTitle->text();
    
    if(docHasChartName(name))
        name = nextChartName(name);

    CrochetTab *tab = createTab();
    
    if(name.isEmpty())
        name = nextChartName();
    
    ui->tabWidget->addTab(tab, name);
    ui->tabWidget->setCurrentWidget(tab);

    QString style = ui->chartStyle->currentText();
    
    tab->createChart(style, rows, cols, defStitch);

    updateMenuItems();
    documentIsModified(true);
}

CrochetTab* MainWindow::createTab()
{

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    
    CrochetTab* tab = new CrochetTab(mEditMode, mStitch, mFgColor, mBgColor, ui->tabWidget);
    tab->setPatternStitches(&mPatternStitches);
    tab->setPatternColors(&mPatternColors);
    connect(tab, SIGNAL(chartStitchChanged()), this, SLOT(updatePatternStitches()));
    connect(tab, SIGNAL(chartColorChanged()), this, SLOT(updatePatternColors()));

    mUndoGroup.addStack(tab->undoStack());
    
    QApplication::restoreOverrideCursor();

    return tab;
}

QString MainWindow::nextChartName(QString baseName)
{
    QString nextName = baseName;

    int i = 1;
    
    while(docHasChartName(nextName)) {
        nextName = baseName + QString::number(i);
        i++;
    }
    
    return nextName;
}

bool MainWindow::docHasChartName(QString name)
{
    int tabCount = ui->tabWidget->count();
    for(int i = 0; i < tabCount; ++i) {
        if (ui->tabWidget->tabText(i) == name)
            return true;
    }

    return false;
}

void MainWindow::viewShowStitches()
{
    ui->allStitchesDock->setVisible(ui->actionShowStitches->isChecked());
}

void MainWindow::viewShowPatternColors()
{
    ui->patternColorsDock->setVisible(ui->actionShowPatternColors->isChecked());
}

void MainWindow::viewShowPatternStitches()
{
    ui->patternStitchesDock->setVisible(ui->actionShowPatternStitches->isChecked());
}

void MainWindow::viewShowUndoHistory()
{
    mUndoDock->setVisible(ui->actionShowUndoHistory->isChecked());
}

void MainWindow::viewShowEditModeToolbar()
{
    ui->editModeToolBar->setVisible(ui->actionShowEditModeToolbar->isChecked());
}

void MainWindow::viewShowMainToolbar()
{
    ui->mainToolBar->setVisible(ui->actionShowMainToolbar->isChecked());
}

void MainWindow::menuModesAboutToShow()
{
    bool enabled = false;
    bool selected = false;
    bool checkedItem = false;
    bool used = false;
    
    QStringList modes;
    if(hasTab()) {
        CrochetTab *tab = qobject_cast<CrochetTab*>(ui->tabWidget->currentWidget());
        modes = tab->editModes();
    }

    foreach(QAction *a, mModeGroup->actions()) {
        if(modes.contains(a->text()))
            enabled = true;
        else
            enabled = false;

        if(mModeGroup->checkedAction() && mModeGroup->checkedAction() == a)
            checkedItem = true;
        
        if(enabled && !used && (!mModeGroup->checkedAction() || checkedItem)) {
            selected = true;
            used = true;
        }
        
        a->setEnabled(enabled);
        a->setChecked(selected);
        selected = false;
    }   
}

void MainWindow::changeTabMode(QAction* a)
{
    int mode = -1;
    
    if(a == ui->actionStitchMode)
        mode = 10;
    else if(a == ui->actionColorMode)
        mode = 11;
    else if(a == ui->actionGridMode)
        mode = 12;
    else if(a == ui->actionPositionMode)
        mode = 13;
    else if(a == ui->actionAngleMode)
        mode = 14;
    else if(a == ui->actionStrechMode)
        mode = 15;
    
    setEditMode(mode);
}

void MainWindow::setEditMode(int mode)
{
    mEditMode = mode;
    
    if(mode == 10)
        ui->actionStitchMode->setChecked(true);
    else if(mode == 11)
        ui->actionColorMode->setChecked(true);
    else if(mode == 12)
        ui->actionGridMode->setChecked(true);
    else if(mode == 13)
        ui->actionPositionMode->setChecked(true);
    else if(mode == 14)
        ui->actionAngleMode->setChecked(true);
    else if(mode == 15)
        ui->actionStrechMode->setChecked(true);
    
    for(int i = 0; i < ui->tabWidget->count(); ++i) {
        CrochetTab *tab = qobject_cast<CrochetTab*>(ui->tabWidget->widget(i));
        if(tab)
            tab->setEditMode(mEditMode);
    }
}

void MainWindow::menuDocumentAboutToShow()
{
    bool state = hasTab();

    ui->actionRemoveTab->setEnabled(state);
}

void MainWindow::menuChartAboutToShow()
{
    bool state = hasTab();
    ui->actionEditName->setEnabled(state);       
}

void MainWindow::chartEditName()
{
    if(!ui->tabWidget->currentWidget())
        return;
    
    int curTab = ui->tabWidget->currentIndex();
    QString currentName  = ui->tabWidget->tabText(curTab);
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Set Chart Name"), tr("Chart name:"),
                                            QLineEdit::Normal, currentName, &ok);
    if(ok && !newName.isEmpty())
        ui->tabWidget->setTabText(curTab, newName);
}

void MainWindow::toolsRegisterSoftware()
{
    if(Settings::inst()->isDemoVersion()) {
        LicenseWizard wizard(true, this);
        if(wizard.exec() == QWizard::Accepted) {
                Settings::inst()->setDemoVersion(false);
                return;
        }
    }
}

void MainWindow::toolsCheckForUpdates()
{
    bool silent = false;
    mUpdater->checkForUpdates(silent);
}

void MainWindow::toolsStitchLibrary()
{
    StitchLibraryUi *d = new StitchLibraryUi(this);
    int value = d->exec();
    
    if(value != QDialog::Accepted) {
        delete d;
        d = 0;
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    
    StitchLibrary::inst()->saveAllSets();

    delete d;
    d = 0;
    
    QApplication::restoreOverrideCursor();
}

void MainWindow::viewZoomIn()
{
    CrochetTab* tab = curCrochetTab();
    if(!tab)
        return;
    tab->zoomIn();
}

void MainWindow::viewZoomOut()
{
    CrochetTab* tab = curCrochetTab();
    if(!tab)
        return;
    tab->zoomOut();
}

CrochetTab* MainWindow::curCrochetTab()
{
    CrochetTab* tab = qobject_cast<CrochetTab*>(ui->tabWidget->currentWidget());
    return tab;
}

bool MainWindow::hasTab()
{
    CrochetTab *cTab = qobject_cast<CrochetTab*>(ui->tabWidget->currentWidget());
    if(!cTab)
        return false;

    return true;
}

QTabWidget* MainWindow::tabWidget()
{
    return ui->tabWidget;
}

void MainWindow::tabChanged(int newTab)
{
    if(newTab == -1)
        return;

    CrochetTab *tab = qobject_cast<CrochetTab*>(ui->tabWidget->widget(newTab));
    if(!tab)
        return;
    
    mUndoGroup.setActiveStack(tab->undoStack());
}

void MainWindow::removeCurrentTab()
{
    removeTab(ui->tabWidget->currentIndex());
}

void MainWindow::removeTab(int tabIndex)
{
    if(tabIndex < 0)
        return;

    //if we're on the last tab close the window.
    if(ui->tabWidget->count() == 1) {
        close();
        return;
    }

    //FIXME: either include a warning that this is NOT undo-able or make it undo-able.
    ui->tabWidget->removeTab(tabIndex);
    documentIsModified(true);
}

void MainWindow::updatePatternStitches()
{
    if(ui->tabWidget->count() <= 0)
        return;

    //FIXME: this whole thing needs to be worked out, but the very least is make this use a shared icon.
    ui->patternStitches->clear();
    QMapIterator<QString, int> i(mPatternStitches);
    while (i.hasNext()) {
        i.next();
        QList<QListWidgetItem*> items = ui->patternStitches->findItems(i.key(), Qt::MatchExactly);
        if(items.count() == 0) {
            Stitch* s = StitchLibrary::inst()->findStitch(i.key());
            QPixmap pix = QPixmap(QSize(32, 32));
            pix.load(s->file());
            QIcon icon = QIcon(pix);
            QListWidgetItem *item = new QListWidgetItem(icon, i.key(), ui->patternStitches);
            ui->patternStitches->addItem(item);
        }
    }
}

void MainWindow::updatePatternColors()
{
    if(ui->tabWidget->count() <= 0)
        return;

    ui->patternColors->clear();

    QString prefix = Settings::inst()->value("colorPrefix").toString();

    QStringList keys = mPatternColors.keys();
    QMap<qint64, QString> sortedColors;
    
    foreach(QString key, keys) {
        qint64 added = mPatternColors.value(key).value("added");
        sortedColors.insert(added, key);
    }

    int i = 1;
    QList<qint64> sortedKeys = sortedColors.keys();
    foreach(qint64 sortedKey, sortedKeys) {
        QString color = sortedColors.value(sortedKey);
        QList<QListWidgetItem*> items = ui->patternColors->findItems(color, Qt::MatchExactly);
        if(items.count() == 0) {
            QPixmap pix = drawColorBox(color, QSize(32, 32));
            QIcon icon = QIcon(pix);
            
            QListWidgetItem *item = new QListWidgetItem(icon, prefix + QString::number(i), ui->patternColors);
            item->setToolTip(color);
            ui->patternColors->addItem(item);
            ++i;
        }
    }
}

void MainWindow::documentIsModified(bool isModified)
{
    //TODO: check all possible modificaiton locations.
    setWindowModified(isModified);
}
