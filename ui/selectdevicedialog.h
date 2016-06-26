//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#ifndef SELECTDEVICEDIALOG_H
#define SELECTDEVICEDIALOG_H
#include "devicing/v4l2device.h"
#include <QDialog>
#include "saveable_widget.h"

namespace Ui {
    class SelectDeviceDialog;
}

class SelectDeviceDialog : public QDialog, protected virtual SaveableWidget
{
    Q_OBJECT

public:
    static v4l2device::device_info pickDevice(QWidget *owner, bool prefferStored); //prefferStored = true means if found last device - use it, instead asking user

protected:

    SelectDeviceDialog() = delete;
    explicit SelectDeviceDialog(QWidget *parent = 0);
    virtual ~SelectDeviceDialog();

    void changeEvent(QEvent *e);
    void updateList(const v4l2device::devices_list_t &devs);
    v4l2device::device_info getSelected();
private slots:
    void on_listDevices_doubleClicked(const QModelIndex &index);

private:
    Ui::SelectDeviceDialog *ui;
    v4l2device::devices_list_t devices;
};

#endif // SELECTDEVICEDIALOG_H
