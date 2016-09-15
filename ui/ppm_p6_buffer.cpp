//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#include "ppm_p6_buffer.h"
#include "utils.h"
#include <linux/videodev2.h>
#include <tuple>
#include <map>
#include <algorithm>

ppm_p6_buffer::ppm_p6_buffer():
    header(),
    buffer(),
    lastW(0),
    lastH(0),
    grey_step(1)
{

}

void ppm_p6_buffer::setBuffer(bool had_change, const uint8_t *mem, size_t size, size_t step)
{
    auto hdrlen = header.length();

    if (buffer.size() < size + hdrlen)
    {
        had_change = true;
        buffer.reserve(size + hdrlen);
        buffer.resize(buffer.capacity());
    }

    if (had_change)
        memcpy(buffer.data(), header.c_str(), hdrlen);

    if (step < 2)
        memcpy(buffer.data() + hdrlen, mem, size);
    else
    {
        for (size_t srcoff = 0, destoff = 0; srcoff < size; srcoff += step, ++destoff)
            *(buffer.data() + hdrlen + destoff) = *(mem + srcoff);

    }
}


void ppm_p6_buffer::set_data(uint32_t w, uint32_t h, const uint8_t *mem, size_t size)
{
    bool had_change = false;
    if (lastW != w || lastH != h || lastSize != size)
    {
        header = utils::string_format("P6\n%u %u\n255\n", w, h);
        had_change = true;
        lastW = w;
        lastH = h;
        lastSize = size;
    }
    setBuffer(had_change, mem, size);
}

void ppm_p6_buffer::set_data_grey8bit(uint32_t w, uint32_t h, const uint8_t *mem, size_t size)
{
    bool had_change = false;
    if (lastW != w || lastH != h || lastSize != size)
    {
        header = utils::string_format("P5\n%u %u\n255\n", w, h);
        had_change = true;
        lastW = w;
        lastH = h;
        lastSize = size;
        grey_step = std::max(static_cast<size_t>(1), lastSize / (w * h));
        //my 64 bit v4l returns 16 bit instead 8bit for V4L2_PIX_FMT_GREY
        //(upd: it returns yuv2 yet - no conversion, updated source to V4L2_PIX_FMT_YUYV so this function will be working)
    }
    setBuffer(had_change, mem, size, grey_step);
}

void ppm_p6_buffer::set_data_p7(uint32_t w, uint32_t h, const uint8_t *mem, size_t size, uint32_t v4lformat)
{
    using rec_t = std::tuple<std::string, int, int>;
    const static std::map<uint32_t, rec_t> formats = {
        {V4L2_PIX_FMT_RGB24, {"RGB", 3, 255}},
        {V4L2_PIX_FMT_Y16_BE, {"GRAYSCALE", 1, 65535}},
    };

    if (!formats.count(v4lformat))
        return;

    bool had_change = false;
    if (lastW != w || lastH != h || lastSize != size)
    {
        const auto& v = formats.at(v4lformat);

        header = utils::string_format("P7\nWIDTH %u\nHEIGHT %u\nDEPTH %u\nMAXVAL %u\nTUPLTYPE %s\nENDHDR\n", w, h, std::get<1>(v), std::get<2>(v), std::get<0>(v).c_str());
        had_change = true;
        lastW = w;
        lastH = h;
        lastSize = size;
    }

    setBuffer(had_change, mem, size);
}

const QPixmap &ppm_p6_buffer::toPixmap()
{
    pxmBuff.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()), "PPM");
    return pxmBuff;
}
