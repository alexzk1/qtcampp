//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#ifndef DEVICEPROPERTIES_H
#define DEVICEPROPERTIES_H

#include <QObject>
#include <QWidget>

#include "devicing/v4l2device.h"
#include "deviceproppanebase.h"

//this class builds GUI to represent device properties.
class V4LDeviceProperties : public DevicePropPaneBase
{
    Q_OBJECT
protected:
    using wlist_t       = std::map<__u32, widgetted_pt>;
    using controls_t    = v4l2device::device_controls;

    wlist_t holder; //holds all objects until this is destroyed
    controls_t controls;

private:
    const QString settings_group;
    QPointer<QLabel> presetLbl;
    void controlValueChanged(const widgetted_pt& p, const v4l2_query_ext_ctrl& c);
    void listControls();
    void listFormats();

    QWidget *connectGUI(const widgetted_pt& ptr);

    static bool isEnabled(const v4l2_query_ext_ctrl& c);
    static bool isEnabled(__u32 flags);
    static bool isNeedUpdate(const v4l2_query_ext_ctrl& c);

public:
    explicit V4LDeviceProperties(const v4l2device::device_info device, QWidget *parent = 0);
    virtual ~V4LDeviceProperties();

    virtual QStringList humanReadableSettings() const override;
protected:
    void resetToDefaultsImpl(); //resets to HW defaults
    void reapplyAllImpl(); //forces settings upload into device (all of them)
    void tryReconnectOnErrorImpl(); //videostream reader must call that in case of error to test if device can be reconnected
    void updateControlsImpl(); //updates controls (active / not) according to current device readings
    void setSubgroupImpl(int id);
};

#endif // DEVICEPROPERTIES_H
