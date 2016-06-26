//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QLabel>

#include "saveable_widget.h"
#include "deviceproperties.h"

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

private:
    Ui::MainWindow *ui;
    void createStatusBar();

    QPointer<QLabel> fpsLabel;
    QPointer<QLabel> connStatusLabel;

    void setStatus(bool on);
    void showFps(int fps);
};

#endif // MAINWINDOW_H
