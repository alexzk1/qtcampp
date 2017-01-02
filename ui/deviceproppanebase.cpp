#include "deviceproppanebase.h"

DevicePropPaneBase::DevicePropPaneBase(QWidget *parent) : QWidget(parent)
{

}

video_camera_ptr DevicePropPaneBase::getCurrDevice() const
{
    return currDevice;
}
