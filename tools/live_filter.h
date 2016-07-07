#ifndef LIVE_FILER_H
#define LIVE_FILER_H
#include <stdint.h>
#include <vector>
#include <memory>


//generic frame filter interface

class ILiveFilter
{
public:
    using RGBVector     = std::vector<uint8_t>;
    const RGBVector &addFrame(const RGBVector& rgb24, size_t w, size_t h)
    {
        return addFrame(rgb24.data(), rgb24.size(), w, h);
    }
    virtual const RGBVector &addFrame(const uint8_t *data, size_t size, size_t w, size_t h) = 0;

    virtual void setFilterQuality(size_t quality) = 0;
    virtual void reset() = 0;
    virtual ~ILiveFilter()
    {
    }
};

#endif // LIVE_FILER_H
