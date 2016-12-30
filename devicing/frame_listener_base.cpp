#include "frame_listener_base.h"

void frame_listener_base::initListener(size_t dest_size)
{
    destBuffer.clear();
    destBuffer.reserve(dest_size);
    destBuffer.resize(destBuffer.capacity(), 0);
    start = std::chrono::steady_clock::now();
}

frame_listener_base::frame_listener_base(const frame_listener_base::frame_receiver &callback):
    callback(callback),
    start(),
    destBuffer()
{

}

int64_t frame_listener_base::duration()
{
    auto nw = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast< TimeT>  (nw - start);
    start = nw;
    return dur.count();
}

bool frame_listener_base::isInitialized()
{
    return destBuffer.size() > 0;
}

frame_listener_base::~frame_listener_base()
{

}

