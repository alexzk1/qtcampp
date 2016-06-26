//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "deviceproperties.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

        auto list = v4l2device::list_attached_devices();
        if (list.size())
        {
            centralWidget()->setLayout(new QVBoxLayout());
            centralWidget()->layout()->addWidget(new DeviceProperties(list.at(0)));
        }
    readSettings(this);
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
