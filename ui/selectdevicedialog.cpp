#include "selectdevicedialog.h"
#include "ui_selectdevicedialog.h"
#include "globalsettings.h"


SelectDeviceDialog::SelectDeviceDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelectDeviceDialog)
{
    ui->setupUi(this);
    readSettings(this);
}

v4l2device::device_info SelectDeviceDialog::pickDevice(QWidget* owner, bool prefferStored)
{
    auto devices = v4l2device::list_attached_devices();
    GlobSaveableTempl<QString, false> stored("LastUsedDeviceName");

    if (prefferStored)
    {
        std::string dev = stored.getCachedValue().toStdString();
        for (const auto& d : devices)
        {
            if (dev == d.devname)
                return d;
        }
    }

    v4l2device::device_info res;

    SelectDeviceDialog dialog(owner);
    dialog.updateList(devices);
    if (QDialog::Accepted == dialog.exec())
    {
        res = dialog.getSelected();
        stored.setCachedValue(res.devname.c_str());
    }

    return res;
}

SelectDeviceDialog::~SelectDeviceDialog()
{
    writeSettings(this);
    delete ui;
}

v4l2device::device_info SelectDeviceDialog::getSelected()
{
    v4l2device::device_info r;
    auto curr = ui->listDevices->currentIndex();
    size_t row = static_cast<size_t>(curr.row());

    if (curr.isValid() && row < devices.size())
        r = devices.at(row);
    return r;
}

void SelectDeviceDialog::changeEvent(QEvent *e)
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

void SelectDeviceDialog::updateList(const v4l2device::devices_list_t &devs)
{
    devices = devs;
    ui->listDevices->clear();

    //ensuring indexes into this list and visual list will be the same
    for (size_t i = 0, sz = devices.size(); i < sz; ++i)
    {
        ui->listDevices->addItem(devices.at(i).devname.c_str());
    }
}
