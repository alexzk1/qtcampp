//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com
#include "ppm_p6_buffer.h"

template<typename... Args>
std::string string_format(const char* fmt, Args... args)
{
    size_t size = snprintf(nullptr, 0ul, fmt, args...);
    std::string buf;
    buf.reserve(size + 1);
    buf.resize(size);
    snprintf(&buf[0], size + 1, fmt, args...);
    return buf;
}

ppm_p6_buffer::ppm_p6_buffer():
    header(),
    buffer(),
    lastW(0),
    lastH(0)
{

}

void ppm_p6_buffer::set_data(uint32_t w, uint32_t h, const uint8_t *mem, size_t size)
{
    bool had_change = false;
    if (lastW != w || lastH != h)
    {
        header = string_format("P6\n%u %u\n255\n", w, h);
        had_change = true;
    }
    auto hdrlen = header.length();

    if (buffer.size() < size + hdrlen)
    {
        had_change = true;
        buffer.reserve(size + hdrlen);
        buffer.resize(buffer.capacity());
    }

    if (had_change)
        memcpy(buffer.data(), header.c_str(), hdrlen);

    memcpy(buffer.data() + hdrlen, mem, size);
}

const QPixmap &ppm_p6_buffer::toPixmap()
{
    pxmBuff.loadFromData(buffer.data(), static_cast<uint32_t>(buffer.size()), "PPM");
    return pxmBuff;
}
