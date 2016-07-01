//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#include <QApplication>
#include <QVBoxLayout>
#include <QPixmap>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "selectdevicedialog.h"
#include "settingsdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    lastPropPane(nullptr),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    readSettings(this);
    setWindowTitle(qApp->applicationName());
    createStatusBar();


    connect(this, &MainWindow::hasFrame, this, [this](const QPixmap& pix, int64_t ms)
    {
        showFps(1000 / ms);
        ui->videoOut->setPixmap(pix);

    },Qt::QueuedConnection); //a must, to resolve cross-thread - it does synchro

    relistIfLost();
}

MainWindow::~MainWindow()
{
    stopVideoCap(); //must ensure thread is stopped and do not send us data, otherwise will crash calling camera_input
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
    relistIfLost();
    if (lastPropPane)
        lastPropPane->reapplyAll();
}

void MainWindow::on_actionReset_triggered()
{
    relistIfLost();
    if (lastPropPane)
        lastPropPane->resetToDefaults();
}

void MainWindow::device_lost()
{
    stopVideoCap(); //that will make automatic resume not posseble even if device will be properly picked
    setStatus(false);
    showFps(0);
}

void MainWindow::device_back()
{
    setStatus(true);
    showFps(0);
    launchVideoCap();
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

void MainWindow::relistIfLost()
{
    //if cable reconnected linux will assign new /dev/video* node most likelly
    //so automatic resume is not possible, need to relist devices
    bool needRelist = true;

    if (lastPropPane)
    {
        auto dev = lastPropPane->getCurrDevice();
        if (dev)
        {
            needRelist = !dev->is_valid_yet();
            if (!needRelist)
                device_back();
        }
    }

    if (needRelist)
        on_actionSelect_Camera_triggered(true);
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

void MainWindow::launchVideoCap()
{
    if (lastPropPane)
    {
        auto dev = lastPropPane->getCurrDevice();
        if (dev && !dev->isCameraRunning())
        {
            dev->cameraInput(std::bind(&MainWindow::camera_input, this, std::placeholders::_1,
                                       std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        }
    }
}

void MainWindow::stopVideoCap()
{
    if (lastPropPane)
    {
        auto dev = lastPropPane->getCurrDevice();
        if (dev)
            dev->stopCameraInput();
    }
}

void MainWindow::camera_input(__u32 w, __u32 h, const uint8_t *mem, size_t size, int64_t ms_per_frame)
{
    frame.set_data(w, h, mem, size);
    emit hasFrame(frame.toPixmap(), ms_per_frame);
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog d(this);
    d.exec();

    //reapplying settings
    if (lastPropPane)
        delete lastPropPane;
    relistIfLost();
}
