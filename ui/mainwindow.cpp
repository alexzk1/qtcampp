//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QPixmap>
#include <QDate>
#include <QDir>
#include <array>
#include <QAction>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "selectdevicedialog.h"
#include "settingsdialog.h"

static const QString nightScheme = "QWidget {background-color: #660000;}";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    lastPropPane(nullptr),
    ui(new Ui::MainWindow),
    checkTimer(new QTimer(this)),
    doASnap(false),
    presetsGroup(new QActionGroup(this)),
    initialPreset(nullptr)
{
    ui->setupUi(this);
    readSettings(this);
    setWindowTitle(qApp->applicationName());
    createStatusBar();
    buildGuiParts();

    ui->videoOut->setAlignment(Qt::AlignCenter);
    connect(this, &MainWindow::hasFrame, this, [this](const QPixmap& pix, int64_t ms)
    {
        static int64_t avr = 0;
        static uint64_t counter = 1;
        if (!(counter % 15))
        {
            if (ms < 1)
                showFps(0);
            else
                showFps(15000 / avr);
            avr = 0;
        }
        else
            avr += ms;
        ++counter;
        ui->videoOut->setPixmap(pix);
        if (doASnap)
        {
            saveSnapshoot(pix);
            doASnap = false;
        }

    },Qt::QueuedConnection); //a must, to resolve cross-thread - it does synchro

    connect(checkTimer, &QTimer::timeout, this, [this]()
    {
        relistIfLost();
    }, Qt::QueuedConnection);

    relistIfLost();
    pereodicTestRunStop();

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
    this->restoreState(settings.value("mainwinstate").toByteArray());
}

void MainWindow::recurseWrite(QSettings &settings, QObject *object)
{
    Q_UNUSED(object);
    settings.setValue("splitter", ui->splitter->saveState());
    settings.setValue("mainwinstate", this->saveState());
}

void MainWindow::on_actionSelect_Camera_triggered(bool prefferStored)
{
    auto dev = SelectDeviceDialog::pickDevice(this, prefferStored);

    if (initialPreset) //if we got re-created device frame - reset preset to default
        initialPreset->setChecked(true);

    //The widget becomes a child of the scroll area, and will be destroyed when the scroll area is deleted
    //or when a new widget is set.
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
    const static QPixmap empty;
    stopVideoCap(); //that will make automatic resume (by polling thread in v4l2device) not possible even if device will be properly picked
    setStatus(false);
    hasFrame(empty, 0);
}

void MainWindow::device_back()
{
    setStatus(true);
    launchVideoCap();
    //presetsGroup->checkedAction()->trigger();
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

void MainWindow::saveSnapshoot(const QPixmap &pxm)
{
    const static std::string formats[]
    {
        "png",
        "ppm",
        "jpg",
    };

    //that will remain the same between program launches, so kinda "night picturing", when you start at 9pm and end-up at 6am -- all will be in same folder
    const static QString dateDir = QDate::currentDate().toString("dd_MMM_yyyy");

    int index = StaticSettingsMap::getGlobalSetts().readInt("Wp0SingleShotFormat");
    QString folder; //for thread-safety will do it here
    StaticSettingsMap::getGlobalSetts().readValue<QString, false>("WorkingFolder", folder);

    auto next = getNextFileId();

    auto executor = [this, index, folder, next, pxm]()
    {
        QString path= folder +"/" +dateDir;
        QDir dir;dir.mkpath(path);
        auto fn = QString("%1/%2.%3").arg(path).arg(next,8,10,QChar('0')).arg(formats[index].c_str());
        pxm.save(fn, formats[index].c_str());
    };

    std::thread tmp(executor);
    tmp.detach();
}

qint64 MainWindow::getNextFileId()
{
    static GlobSaveableTempl<qint64, true> fileNamer("FileNamerCounter", 0);
    qint64 next = fileNamer.getCachedValue();
    fileNamer.setCachedValue(next + 1);
    fileNamer.flush(); //important counter, don't want to loose on crash
    return next;
}

void MainWindow::relistIfLost()
{
    //if cable reconnected linux will assign new /dev/video* node most likelly
    //so automatic resume is not possible, need to relist devices
    //but timer may tick too fast while user has GUI popped - stop that
    static std::atomic_flag alreadyOpened = ATOMIC_FLAG_INIT;
    if (!alreadyOpened.test_and_set())
    {
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
        alreadyOpened.clear();
    }
}

void MainWindow::forceRelist()
{
    if (lastPropPane)
        delete lastPropPane;
    relistIfLost();
}

void MainWindow::pereodicTestRunStop()
{
    if (checkTimer)
    {
        if (StaticSettingsMap::getGlobalSetts().readBool("PereodicDeviceTestPresence"))
            checkTimer->start(1000 * StaticSettingsMap::getGlobalSetts().readInt("PereodicDeviceTestSecs"));
        else
            checkTimer->stop();
    }
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
            doASnap = false;
            dev->cameraInput(std::bind(&MainWindow::camera_input, this, std::placeholders::_1,
                                       std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
        }
    }
}

void MainWindow::stopVideoCap()
{
    doASnap = false;
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

void MainWindow::buildGuiParts()
{
    if (presetsGroup)
    {
        for (int i = 1 ; i < 11; ++i)
        {
            QString ks = QString("alt+%1").arg(i % 10);
            auto a = ui->menuSettings->addAction(tr("Camera Preset %1").arg(i));
            a->setShortcut(ks);
            connect(a, &QAction::triggered, this, [this, i]()
            {
                if (lastPropPane)
                {
                    lastPropPane->setSubgroup(i - 1);
                }
            }, Qt::QueuedConnection);
            a->setCheckable(true);
            if (i == 1)
            {
                a->setChecked(true);
                initialPreset = a;
            }
            presetsGroup->addAction(a);
        }
    }
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog d(this);
    d.exec();

    //reapplying settings
    forceRelist();
    pereodicTestRunStop();
}

void MainWindow::on_actionFullscreen_triggered(bool checked)
{
    Q_UNUSED(checked);
    setWindowState(windowState() ^ Qt::WindowFullScreen);
}

void MainWindow::on_actionNight_Mode_triggered(bool checked)
{
    if (checked)
        qApp->setStyleSheet(nightScheme);
    else
        qApp->setStyleSheet("");
}

void MainWindow::on_actionSingleShoot_triggered()
{
    doASnap = true;
}
