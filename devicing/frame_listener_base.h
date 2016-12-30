#ifndef FRAME_LISTENER_BASE_H
#define FRAME_LISTENER_BASE_H

#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <stdint.h>

class frame_listener_base
{
public:
    //callback function, assigned to this listener
    using frame_receiver = std::function<void (uint32_t w, uint32_t h, const uint8_t* memory, size_t length,  int64_t ms_per_frame, uint32_t format)>;
protected:
    using TimeT = std::chrono::milliseconds;
    const frame_receiver callback;
    std::chrono::steady_clock::time_point start;
    std::vector<uint8_t> destBuffer;


    void initListener(size_t dest_size);
public:
    frame_listener_base(const frame_receiver& callback);

    frame_listener_base() = delete;
    frame_listener_base(const frame_listener_base&) = delete;
    frame_listener_base(frame_listener_base&&) = delete;
    frame_listener_base& operator =(const frame_listener_base&) = delete;
    frame_listener_base& operator =(frame_listener_base&&) = delete;

    virtual ~frame_listener_base();
    int64_t duration();
    virtual bool isInitialized();
};

using frame_listener_ptr = std::shared_ptr<frame_listener_base>;

#endif // FRAME_LISTENER_BASE_H
