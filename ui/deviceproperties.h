//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#ifndef DEVICEPROPERTIES_H
#define DEVICEPROPERTIES_H

#include <QObject>
#include <QWidget>
#include <QString>
#include <vector>
#include <map>


#include "saveable_widget.h"
#include "devicing/v4l2device.h"
#include "globalsettings.h"

//this class builds GUI to represent device properties.
class DeviceProperties : public QWidget, protected virtual SaveableWidget
{
    Q_OBJECT
protected:
    using widgetted_pt  = std::shared_ptr<ISaveableWidget>;
    using widgetted_ptw = std::weak_ptr<ISaveableWidget>;
    using wlist_t       = std::map<__u32, widgetted_pt>;
    using controls_t    = v4l2device::device_controls;

    wlist_t holder; //holds all objects until this is destroyed
    v4l2device_ptr currDevice;
    controls_t controls;
private:
    const QString settings_group;
    void controlValueChanged(const widgetted_pt& p, const v4l2_query_ext_ctrl& c);
    void listControls();
    void listFormats();

    QWidget *connectGUI(const v4l2_query_ext_ctrl &c, const widgetted_pt& ptr);

    static bool isEnabled(const v4l2_query_ext_ctrl& c);
    static bool isEnabled(__u32 flags);
    static bool isNeedUpdate(const v4l2_query_ext_ctrl& c);

public:
    explicit DeviceProperties(const v4l2device::device_info device, QWidget *parent = 0);

    v4l2device_ptr getCurrDevice() const;

signals:
    void deviceDisconnected();
    void deviceRestored();
public slots:
    void resetToDefaults(); //resets to HW defaults
    void reapplyAll(); //forces settings upload into device (all of them)
    void tryReconnectOnError(); //videostream reader must call that in case of error to test if device can be reconnected
    void updateControls(); //updates controls (active / not) according to current device readings
};

#endif // DEVICEPROPERTIES_H
