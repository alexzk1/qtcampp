#ifndef FRAME_LISTENER_H
#define FRAME_LISTENER_H

#include "frame_listener_base.h"
#include <libv4lconvert.h>


//this class is listener for the frames, which performs needed conversion(s)
//it can be couple listeners per device attached (for example, RGB for snapshoots and YUV for video at the same moment)
class frame_listener_v4l : public frame_listener_base
{
public:

private:
    friend class v4l2device;

    const uint32_t       targetFormat;
    std::shared_ptr<v4lconvert_data> converter;
    v4l2_format destFormat;

    void setNextFrame(const v4l2_format& srcFormat, const uint8_t* memory, size_t length);
    size_t initConverter(int fd, const v4l2_format& srcFormat); //returns destination format buffer size
public:
    frame_listener_v4l(const frame_receiver& callback, uint32_t targetFormat);

    frame_listener_v4l() = delete;
    frame_listener_v4l(const frame_listener_v4l&) = delete;
    frame_listener_v4l(frame_listener_v4l&&) = delete;
    frame_listener_v4l& operator =(const frame_listener_v4l&) = delete;
    frame_listener_v4l& operator =(frame_listener_v4l&&) = delete;
    virtual ~frame_listener_v4l();

    virtual bool isInitialized() override;
};


#endif // FRAME_LISTENER_H
