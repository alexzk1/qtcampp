//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "deviceproperties.h"
#include <QVBoxLayout>
#include "selectdevicedialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    readSettings(this);
    auto dev = SelectDeviceDialog::pickDevice(this, true);
    centralWidget()->setLayout(new QVBoxLayout());
    centralWidget()->layout()->addWidget(new DeviceProperties(dev));
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
