#include "frame_listener_v4l.h"
#include <memory.h>
#include <iostream>

#include <linux/videodev2.h>

frame_listener_v4l::frame_listener_v4l(const frame_receiver &callback, uint32_t targetFormat):
    frame_listener_base(callback),
    targetFormat(targetFormat),
    converter(nullptr),
    destFormat()
{
    memset(&destFormat, 0, sizeof(v4l2_format));
}

frame_listener_v4l::~frame_listener_v4l()
{

}

bool frame_listener_v4l::isInitialized()
{
    return converter != nullptr && frame_listener_base::isInitialized();
}

void frame_listener_v4l::setNextFrame(const v4l2_format& srcFormat, const uint8_t* memory, size_t length)
{
    if (isInitialized())
    {
        int size = static_cast<int>(destBuffer.size());
        size = v4lconvert_convert(converter.get(), &srcFormat,
                                  &destFormat, const_cast<uint8_t*>(memory), static_cast<int>(length), destBuffer.data(), static_cast<int>(destBuffer.size()));

        if (size > -1)

        {
            //be aware that we output pure pixels there, so listener must add proper headers to load as image
            callback(destFormat.fmt.pix.width, destFormat.fmt.pix.height, destBuffer.data(),
                     static_cast<size_t>(size), duration(), destFormat.fmt.pix.pixelformat);
        }
        else
            std::cerr << v4lconvert_get_error_message(converter.get()) <<std::endl;
    }
    else
        std::cerr << "Converter is not initialized or buffer not allocated." << std::endl;
}

size_t frame_listener_v4l::initConverter(int fd, const v4l2_format &srcFormat)
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
        initListener(destFormat.fmt.pix.sizeimage); //lib properly calculates buffer size in my case
    }
    return destBuffer.size();
}


