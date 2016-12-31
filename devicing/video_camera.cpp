#include "video_camera.h"

video_camera::video_camera()
{

}

video_camera::~video_camera()
{

}

void video_camera::setNamedListener(const std::string &name, const frame_listener_ptr &listener)
{
    namedListeners.set(name, listener);
}

void video_camera::startCameraInput()
{
    if (is_valid_yet() && !thr)
    {
        thr = utility::startNewRunner(getInputFunction());
    }
}

void video_camera::stopCameraInput()
{
    thr.reset();
}

bool video_camera::isCameraRunning() const
{
    return thr != nullptr;
}
