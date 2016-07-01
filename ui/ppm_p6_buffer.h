#ifndef PPM_P6_BUFFER_H
#define PPM_P6_BUFFER_H
//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#include <stdint.h>
#include <vector>
#include <memory>
#include <string>
#include <QImage>
#include <QPixmap>

//simple stuff - adds PPM format header to RGB24 data, so can be loaded / saved by qt classes

class ppm_p6_buffer
{
private:
    std::string header;
    std::vector<uint8_t>    buffer;
    uint32_t lastW;
    uint32_t lastH;
    QPixmap pxmBuff;
public:
    ppm_p6_buffer();
    void set_data(uint32_t w, uint32_t h, const uint8_t* mem, size_t size);
    const QPixmap& toPixmap();
};

#endif // PPM_P6_BUFFER_H
