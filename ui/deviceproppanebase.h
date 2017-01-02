#ifndef DEVICEPROPPANEBASE_H
#define DEVICEPROPPANEBASE_H

#include <QString>
#include <QLabel>
#include <vector>
#include <map>
#include <stdint.h>

#include "devicing/video_camera.h"
#include "saveable_widget.h"
#include "globalsettings.h"

//not sure if qt allows virtual slots, lets do this
#define VIRT_SLOT_V(FN) public slots: void FN(){FN##Impl();} protected: virtual void FN##Impl() = 0
#define VIRT_SLOT_I(FN) public slots: void FN(int i){FN##Impl(i);} protected: virtual void FN##Impl(int i) = 0

class DevicePropPaneBase : public QWidget, protected virtual SaveableWidget
{
    Q_OBJECT
protected:
    using widgetted_pt  = std::shared_ptr<ISaveableWidget>;
    using widgetted_ptw = std::weak_ptr<ISaveableWidget>;

    video_camera_ptr currDevice;
    std::atomic<uint32_t> currentPixelFormatSelected;
public:
    explicit DevicePropPaneBase(QWidget *parent = 0);
    video_camera_ptr getCurrDevice() const;

    virtual QStringList humanReadableSettings() const = 0;
    uint32_t getSelectedDevicePixelFormat(){return currentPixelFormatSelected;}
signals:
    void deviceDisconnected();
    void deviceRestored();

public:
    VIRT_SLOT_V(resetToDefaults); //resets to HW defaults
    VIRT_SLOT_V(reapplyAll); //forces settings upload into device (all of them)
    VIRT_SLOT_V(tryReconnectOnError); //videostream reader must call that in case of error to test if device can be reconnected
    VIRT_SLOT_V(updateControls); //updates controls (active / not) according to current device readings
    VIRT_SLOT_I(setSubgroup);
};

#undef VIRT_SLOT_V
#undef VIRT_SLOT_I

#endif // DEVICEPROPPANEBASE_H
