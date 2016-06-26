//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#ifndef DEVICEPROPERTIES_H
#define DEVICEPROPERTIES_H

#include <QObject>
#include <QWidget>
#include <vector>
#include <map>


#include "saveable_widget.h"
#include "devicing/v4l2device.h"
#include "globalsettings.h"

//this class builds GUI to represent device properties.
class DeviceProperties : public QWidget, protected SaveableWidget
{
    Q_OBJECT
private:

protected:
    using widgetted_pt  = std::shared_ptr<ISaveableWidget>;
    using widgetted_ptw = std::weak_ptr<ISaveableWidget>;
    using wlist_t       = std::map<__u32, widgetted_pt>;
    using controls_t    = v4l2device::device_controls;

    wlist_t holder; //holds all objects until this is destroyed
    v4l2device_ptr currDevice;
    controls_t controls;
public:
    explicit DeviceProperties(const v4l2device::device_info device, QWidget *parent = 0);
    v4l2device_ptr getCurrDevice() const;

signals:
    void deviceDisconnected();
    void deviceRestored();
public slots:
    void resetToDefaults();
    void reapplyAll();
    void tryReconnectOnError(); //videostream reader must call that in case of error to test if device can be reconnected
    void updateControls();
};

#endif // DEVICEPROPERTIES_H
