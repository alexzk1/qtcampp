//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QPixmap>
#include <QDate>
#include <QDir>
#include <array>
#include <QAction>
#include <QFile>
#include <QTextStream>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "selectdevicedialog.h"
#include "settingsdialog.h"

#ifdef CAMPP_TOOLS_USED
#include "tools/onlinestacker.h"
#endif

static const QString nightScheme = "QWidget {background-color: #660000;}";

#define GREYSCALE StaticSettingsMap::getGlobalSetts().readBool<true>("Use_greyscale")

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    lastPropPane(nullptr),
    ui(new Ui::MainWindow),
    checkTimer(new QTimer(this)),
    doASnap(false),
    doASeries(false),
    useFilters(false),
    presetsGroup(new QActionGroup(this))
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

        static int     seriesLimit = 0;
        static int     seriesCount = 0;
        static qint64  seriesID = -1;
        static QString path;
        if (doASeries)
        {
            doASnap = false;

            if (ui->actionSeries_Shoot->isEnabled())
            {
                seriesID = getNextSeriesId();
                path     = savingPath(seriesID);

                ui->actionSeries_Shoot->setEnabled(false);
                seriesCount = 0;
                seriesLimit = StaticSettingsMap::getGlobalSetts().readInt("Wp0SeriesLen");

                if (lastPropPane)
                {
                    QFile txt(QString("%1/used_settings.txt").arg(path));
                    txt.open(QIODevice::WriteOnly | QIODevice::Text);
                    QTextStream out(&txt);
                    auto sl = lastPropPane->readAllValues();
                    for (const auto& s : sl)
                        out << s <<"\n";
                }

            }

            if (seriesCount++ < seriesLimit)
                saveSnapshoot(pix, path);
            else
            {
                doASeries = false;
                ui->actionSeries_Shoot->setEnabled(true);
                statusBar()->showMessage(tr("Finished. Couple pictures were taken at %1").arg(path), 15000);
            }
        }

        if (doASnap)
        {
            saveSnapshoot(pix, savingPath(-1));
            doASnap = false;
        }

    },Qt::QueuedConnection); //a must, to resolve cross-thread - it does synchro

    connect(checkTimer, &QTimer::timeout, this, [this]()
    {
        relistIfLost();
    }, Qt::QueuedConnection);

    relistIfLost();
    pereodicTestRunStop();
    buildFilters();
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

void MainWindow::saveSnapshoot(const QPixmap &pxm, const QString& path)
{
    const static std::string formats[]
    {
        "png",
        "ppm",
        "jpg",
    };

    int index = StaticSettingsMap::getGlobalSetts().readInt("Wp0SingleShotFormat");

    QString fi;
    auto next = getNextFileId();
    fi = QString("%1.%2").arg(next,8,10,QChar('0')).arg(formats[index].c_str());


    auto executor = [this, index, fi, pxm, path]()
    {
        auto fn = QString("%1/%2").arg(path).arg(fi);
        pxm.save(fn, formats[index].c_str());
    };

    std::thread tmp(executor);
    tmp.detach();
}

QString MainWindow::savingPath(qint64 series)
{
    //that will remain the same between program launches, so kinda "night picturing", when you start at 9pm and end-up at 6am -- all will be in same folder
    const static QString dateDir = QDate::currentDate().toString("dd_MMM_yyyy");

    QString folder; //for thread-safety will do it here
    StaticSettingsMap::getGlobalSetts().readValue<QString, false>("WorkingFolder", folder);
    QString path= folder +"/" +dateDir;
    if (series > -1)
    {
        path = path + QString("/ser_%1").arg(series,4,10,QChar('0'));
    }
    QDir dir;dir.mkpath(path);
    return path;
}

qint64 MainWindow::getNextFileId()
{
    static GlobSaveableTempl<qint64, true> fileNamer("FileNamerCounter", 0);
    qint64 next = fileNamer.getCachedValue();
    fileNamer.setCachedValue(next + 1);
    fileNamer.flush(); //important counter, don't want to loose on crash
    return next;
}

qint64 MainWindow::getNextSeriesId()
{
    static GlobSaveableTempl<qint64, true> fileNamer("SeriesFileNamerCounter", 0);
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
        if (lastSubaction)
            lastSubaction->setChecked(true);
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
                                       std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6),
                             (GREYSCALE)?V4L2_PIX_FMT_GREY:V4L2_PIX_FMT_RGB24);
                             //(GREYSCALE)?V4L2_PIX_FMT_Y16_BE:V4L2_PIX_FMT_RGB24);
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

void MainWindow::camera_input(__u32 w, __u32 h, const uint8_t *mem, size_t size, int64_t ms_per_frame, uint32_t pxl_format)
{
    Q_UNUSED(pxl_format); //todo: for now, need to figure how to display P7 format in qt (which can do 16 bit greyscale needed for very dark stars like 12-14 star size)
    auto pm = mem;
#ifdef CAMPP_TOOLS_USED
    if (useFilters && filters.size())
    {
        for (auto& f : filters)
            pm = f->addFrame(pm, size, w, h).data();
#endif
    }
    if (GREYSCALE)
        frame.set_data_grey8bit(w, h, pm, size);
    else
        frame.set_data(w, h, pm, size);
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
            connect(a, &QAction::triggered, this, [this, i, a]()
            {
                if (lastPropPane)
                {
                    lastPropPane->setSubgroup(i - 1);
                    lastSubaction = a;
                }
            }, Qt::QueuedConnection);
            a->setCheckable(true);
            if (i == 1)
            {
                a->setChecked(true);
            }
            presetsGroup->addAction(a);
        }
    }
}


void MainWindow::buildFilters()
{
#ifdef CAMPP_TOOLS_USED
    filters.clear();
    //if position of filters is important - this func must take care of
    auto q1 = static_cast<size_t>(StaticSettingsMap::getGlobalSetts().readInt("NoiseFilter"));
    if (q1)
    {
        auto f1 = std::make_shared<OnlineStacker>();
        f1->setFilterQuality(q1);
        filters.push_back(f1);
    }
#endif
}


void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog d(this);
    d.exec();

    //reapplying settings
    forceRelist();
    pereodicTestRunStop();

    buildFilters();
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

void MainWindow::on_actionSeries_Shoot_triggered()
{
    statusBar()->showMessage(tr("Doing multiply shoots..."), 5000);
    doASeries = true;
}

void MainWindow::on_actionApply_All_triggered()
{
    forceRelist();
    if (lastPropPane)
        lastPropPane->reapplyAll();
}

void MainWindow::on_actionReset_triggered()
{
    forceRelist();
    if (lastPropPane)
        lastPropPane->resetToDefaults();
}

void MainWindow::on_actionEnable_Filter_s_triggered(bool checked)
{
    useFilters = checked;
#ifdef CAMPP_TOOLS_USED
    for (auto& f : filters)
        f->reset();
#endif
}
