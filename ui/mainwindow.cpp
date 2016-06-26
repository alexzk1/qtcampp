//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#include <QApplication>
#include <QVBoxLayout>
#include <QPixmap>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "selectdevicedialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    lastPropPane(nullptr),
    ui(new Ui::MainWindow)

{
    ui->setupUi(this);
    readSettings(this);
    setWindowTitle(qApp->applicationName());
    createStatusBar();

    on_actionSelect_Camera_triggered(true);
}

MainWindow::~MainWindow()
{
    writeSettings(this);
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void MainWindow::recurseRead(QSettings &settings, QObject *object)
{
    Q_UNUSED(object); //not really recursion (or tree walk)
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
}

void MainWindow::recurseWrite(QSettings &settings, QObject *object)
{
    Q_UNUSED(object);
    settings.setValue("splitter", ui->splitter->saveState());
}

void MainWindow::on_actionSelect_Camera_triggered(bool prefferStored)
{
    auto dev = SelectDeviceDialog::pickDevice(this, prefferStored);
    ui->scrollControls->setWidget(lastPropPane = new DeviceProperties(dev));

    connect(lastPropPane, &DeviceProperties::deviceDisconnected, this, &MainWindow::device_lost, Qt::QueuedConnection);
    connect(lastPropPane, &DeviceProperties::deviceRestored,     this, &MainWindow::device_back, Qt::QueuedConnection);

    //initial gui show
    auto devp = lastPropPane->getCurrDevice();
    if(devp && devp->is_valid_yet())
        device_back();
    else
        device_lost();
}

void MainWindow::on_actionApply_All_triggered()
{
    if (lastPropPane)
        lastPropPane->reapplyAll();
}

void MainWindow::on_actionReset_triggered()
{
    if (lastPropPane)
        lastPropPane->resetToDefaults();
}

void MainWindow::device_lost()
{
    setStatus(false);
    showFps(0);
}

void MainWindow::device_back()
{
    setStatus(true);
    showFps(0);
}

void MainWindow::createStatusBar()
{
    connStatusLabel = new QLabel();
    fpsLabel = new QLabel();
    statusBar()->addPermanentWidget(connStatusLabel);
    statusBar()->addWidget(fpsLabel);
    setStatus(false);
    showFps(0);
}

void MainWindow::setStatus(bool on)
{
    static const QPixmap conoff = QPixmap(":/icons/camoff").scaledToHeight(22);
    static const QPixmap conon  = QPixmap(":/icons/camon").scaledToHeight(22);
    if (connStatusLabel)
    {
        connStatusLabel->setPixmap((on)?conon:conoff);
        connStatusLabel->setToolTip((on)?tr("Device is connected."):tr("Lost connection to the device."));
    }
}

void MainWindow::showFps(int fps)
{
    if (fpsLabel)
        fpsLabel->setText(tr("FPS: %1").arg(fps));
}
