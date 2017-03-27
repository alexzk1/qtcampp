#include "v4l_selectdevicedialog.h"
#include "ui_v4l_selectdevicedialog.h"
#include "globalsettings.h"
//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

V4LSelectDeviceDialog::V4LSelectDeviceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::V4LSelectDeviceDialog)
{
    ui->setupUi(this);
    readSettings(this);
}

v4l2device::device_info V4LSelectDeviceDialog::pickDevice(QWidget* owner, bool prefferStored)
{
    auto devices = v4l2device::list_attached_devices();
    GlobSaveableTempl<QString, false> stored("LastUsedDeviceName");

    if (prefferStored)
    {
        const std::string dev = stored.getCachedValue().toStdString();
        for (const auto& d : devices)
        {
            if (dev == d.devname)
                return d;
        }
    }

    v4l2device::device_info res;
    if (devices.size() || !prefferStored) //if prefferedStored but no devices connected - do not popup
    {
        V4LSelectDeviceDialog dialog(owner);
        dialog.updateList(devices);
        if (QDialog::Accepted == dialog.exec())
        {
            res = dialog.getSelected();

            if (!res.devname.empty()) //if empty selected - do not cache it
                stored.setCachedValue(res.devname.c_str());
        }
    }
    return res;
}

V4LSelectDeviceDialog::~V4LSelectDeviceDialog()
{
    writeSettings(this);
    delete ui;
}

v4l2device::device_info V4LSelectDeviceDialog::getSelected()
{
    v4l2device::device_info r;
    auto curr = ui->listDevices->currentIndex();
    size_t row = static_cast<size_t>(curr.row());

    if (curr.isValid() && row < devices.size())
        r = devices.at(row);
    return r;
}

void V4LSelectDeviceDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

void V4LSelectDeviceDialog::updateList(const v4l2device::devices_list_t &devs)
{
    devices = devs;
    ui->listDevices->clear();

    //ensuring indexes into this list and visual list will be the same
    for (size_t i = 0, sz = devices.size(); i < sz; ++i)
    {
        ui->listDevices->addItem(devices.at(i).devname.c_str());
    }
}

void V4LSelectDeviceDialog::on_listDevices_doubleClicked(const QModelIndex &index)
{
    if (index.isValid())
        accept();
}
