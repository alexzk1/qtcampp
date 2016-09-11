#include "frame_listener.h"
#include <linux/videodev2.h>
#include <memory.h>
#include <iostream>

frame_listener::frame_listener(const frame_receiver &callback, uint32_t targetFormat):
    callback(callback),
    targetFormat(targetFormat),
    converter(nullptr),
    destFormat(),
    destBuffer(),
    start()
{
    memset(&destFormat, 0, sizeof(v4l2_format));
}

frame_listener::~frame_listener()
{

}

bool frame_listener::isInitialized()
{
    return converter != nullptr && destBuffer.size() > 0;
}

void frame_listener::setNextFrame(const v4l2_format& srcFormat, const uint8_t* memory, size_t length)
{
    if (isInitialized())
    {
        int size = static_cast<int>(destBuffer.size());
        size = v4lconvert_convert(converter.get(), &srcFormat,
                                  &destFormat, const_cast<uint8_t*>(memory), static_cast<int>(length), destBuffer.data(), static_cast<int>(destBuffer.size()));

        if (size > -1)

        {
            auto duration = std::chrono::duration_cast< TimeT>  (std::chrono::steady_clock::now() - start);
            start = std::chrono::steady_clock::now();

            //be aware that we output pure pixels there, so listener must add proper headers to load as image
            callback(destFormat.fmt.pix.width, destFormat.fmt.pix.height, destBuffer.data(),
                     static_cast<size_t>(size), duration.count(), destFormat.fmt.pix.pixelformat);

        }
        else
            std::cerr << v4lconvert_get_error_message(converter.get()) <<std::endl;
    }
    else
        std::cerr << "Converter is not initialized or buffer not allocated." << std::endl;
}

size_t frame_listener::initConverter(int fd, const v4l2_format &srcFormat)
{
    destBuffer.clear();
    //cleansing structs, guess driver will change only some bits
    memset(&destFormat, 0, sizeof(v4l2_format));

    converter.reset(v4lconvert_create(fd), [](v4lconvert_data* p)
    {
        if (p)
            v4lconvert_destroy(p);
    });
    if (converter)
    {
        destFormat = srcFormat;
        destFormat.fmt.pix.pixelformat = targetFormat;
        v4lconvert_try_format(converter.get(), &destFormat, nullptr);
        destBuffer.reserve(destFormat.fmt.pix.sizeimage); //lib properly calculates buffer size in my case
        destBuffer.resize(destBuffer.capacity(), 0);
        start = std::chrono::steady_clock::now();
    }
    return destBuffer.size();
}
