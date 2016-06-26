//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include "ui/mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QStringList>

#include "devicing/v4l2device.h"

int main(int argc, char *argv[])
{
        QCoreApplication::setOrganizationName("pasteover.net");
        QCoreApplication::setOrganizationDomain("pasteover.net");
        QCoreApplication::setApplicationName("QtCamPP");

        QApplication a(argc, argv);
        MainWindow w;
        w.show();

        return a.exec();
//    auto list = v4l2device::list_attached_devices();
//    if (list.size())
//    {
//        auto dev = list.at(0).open();
//        if (dev)
//        {
//            auto controls = dev->listControls();
//            for (const auto& c : controls)
//            {
//                qDebug()<<c.name;
//            }
//        }
//    }
//    return 0;
}
