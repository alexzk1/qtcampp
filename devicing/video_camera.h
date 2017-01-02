#ifndef VIDEO_CAMERA_H
#define VIDEO_CAMERA_H

#include <memory>
#include "guardeds.h"
#include "runners.h"
#include "frame_listener_base.h"

class video_camera
{
private:
    utility::runner_t thr;
protected:
    using listeners_holder_t = guarded_map<std::string, frame_listener_ptr>;
    listeners_holder_t namedListeners;
    virtual utility::runner_f_t getInputFunction() = 0;
public:
    video_camera(){}
    video_camera(const video_camera&) = delete;
    video_camera& operator= (const video_camera&) = delete;
    virtual ~video_camera(){}

    void setNamedListener(const std::string& name, const frame_listener_ptr& listener) //2nd parameter as nullptr will remove it
    {
        namedListeners.set(name, listener);
    }

    //device's state (opened, closed)
    virtual bool open(const std::string& device) = 0;
    virtual bool reopen() = 0; //tries reopen
    virtual void close() = 0;
    virtual bool is_valid_yet() const = 0; //tests if device is connected/opened yet


    //multithreaded camera capturing, setting
    void startCameraInput()
    {
        if (is_valid_yet() && !thr)
        {
            thr = utility::startNewRunner(getInputFunction());
        }
    }

    void stopCameraInput()
    {
        thr.reset();
    }

    bool isCameraRunning() const
    {
        return thr != nullptr;
    }

    template<class ChildT>
    ChildT* as()
    {
        return dynamic_cast<ChildT*>(this);
    }

    template<class ChildT>
    ChildT* as() const
    {
        return dynamic_cast<const ChildT*>(this);
    }
};

using  video_camera_ptr = std::shared_ptr<video_camera>;

#endif // VIDEO_CAMERA_H
