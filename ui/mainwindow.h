//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "saveable_widget.h"

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
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
