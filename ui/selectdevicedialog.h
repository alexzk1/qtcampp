#ifndef SELECTDEVICEDIALOG_H
#define SELECTDEVICEDIALOG_H
#include "devicing/v4l2device.h"
#include <QDialog>
#include "saveable_widget.h"

namespace Ui {
    class SelectDeviceDialog;
}

class SelectDeviceDialog : public QDialog, protected SaveableWidget
{
    Q_OBJECT

public:
    static v4l2device::device_info pickDevice(QWidget *owner, bool prefferStored);
    ~SelectDeviceDialog();


protected:
    explicit SelectDeviceDialog(QWidget *parent = 0);
    void changeEvent(QEvent *e);
    void updateList(const v4l2device::devices_list_t &devs);
    v4l2device::device_info getSelected();
private:
    Ui::SelectDeviceDialog *ui;
    v4l2device::devices_list_t devices;
};

#endif // SELECTDEVICEDIALOG_H
