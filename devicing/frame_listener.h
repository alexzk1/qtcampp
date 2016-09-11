#ifndef FRAME_LISTENER_H
#define FRAME_LISTENER_H

#include <memory>
#include <functional>
#include <vector>
#include <chrono>

#include <stdint.h>
#include <libv4lconvert.h>


//this class is listener for the frames, which performs needed conversion(s)
//it can be couple listeners per device attached (for example, RGB for snapshoots and YUV for video at the same moment)

class frame_listener
{
public:
    //callback function, assigned to this listener
    using frame_receiver = std::function<void (uint32_t w, uint32_t h, const uint8_t* memory, size_t length,  int64_t ms_per_frame, uint32_t format)>;
private:
    friend class v4l2device;
    using TimeT = std::chrono::milliseconds;

    const frame_receiver callback;
    const uint32_t       targetFormat;
    std::shared_ptr<v4lconvert_data> converter;
    v4l2_format destFormat;
    std::vector<uint8_t> destBuffer;
    std::chrono::steady_clock::time_point start;

    void setNextFrame(const v4l2_format& srcFormat, const uint8_t* memory, size_t length);
    size_t initConverter(int fd, const v4l2_format& srcFormat); //returns destination format buffer size
public:
    frame_listener(const frame_receiver& callback, uint32_t targetFormat);

    frame_listener() = delete;
    frame_listener(const frame_listener&) = delete;
    frame_listener(frame_listener&&) = delete;
    frame_listener& operator =(const frame_listener&) = delete;
    frame_listener& operator =(frame_listener&&) = delete;
    virtual ~frame_listener();

    bool isInitialized();
};

using frame_listener_ptr = std::shared_ptr<frame_listener>;

#endif // FRAME_LISTENER_H
