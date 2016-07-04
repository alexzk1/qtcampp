//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QLabel>
#include <QPixmap>
#include <memory>
#include <QTimer>
#include <atomic>

#include "saveable_widget.h"
#include "deviceproperties.h"
#include "ppm_p6_buffer.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow, protected SaveableWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    QPointer<DeviceProperties> lastPropPane;

    void changeEvent(QEvent *e);
    virtual void recurseRead(QSettings& settings, QObject* object) override;
    virtual void recurseWrite(QSettings& settings, QObject* object) override;
private slots:
    void on_actionSelect_Camera_triggered(bool prefferStored = false);

    void on_actionApply_All_triggered();

    void on_actionReset_triggered();

    void device_lost();
    void device_back();
    void on_actionSettings_triggered();
    void on_actionFullscreen_triggered(bool checked);

    void on_actionNight_Mode_triggered(bool checked);

    void on_actionSingleShoot_triggered();

signals:
    void hasFrame(QPixmap pix, int64_t ms_per_frame); //used to resolve cross thread from puller to GUI
private:
    Ui::MainWindow *ui;
    void createStatusBar();

    QPointer<QLabel> fpsLabel;
    QPointer<QLabel> connStatusLabel;
    ppm_p6_buffer frame;
    QPointer<QTimer> checkTimer;
    std::atomic<bool> doASnap;

    void saveSnapshoot(const QPixmap& pxm);
    void relistIfLost();
    void forceRelist();
    void pereodicTestRunStop();

    void setStatus(bool on);
    void showFps(int fps);

    void launchVideoCap();
    void stopVideoCap();
    void camera_input(__u32 w, __u32 h, const uint8_t* mem, size_t size,  int64_t ms_per_frame);
};

#endif // MAINWINDOW_H
