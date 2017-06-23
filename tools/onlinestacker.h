#ifndef ONLINESTUCKER_H
#define ONLINESTUCKER_H
#include <stdint.h>
#include <vector>
#include <deque>
#include <memory>
#include "live_filter.h"

//simple linear frame's addition - allows to lower random noise (rises SNR)
class OnlineStacker : public ILiveFilter
{
public:

    using RGBVectorSum  = std::vector<uint64_t>;
    using QueuedElement = RGBVector;
    using RGBQueue      = std::deque<std::shared_ptr<QueuedElement>>;
private:
    RGBVectorSum   summ;
    RGBQueue       images;
    RGBVector      outImage;
    void initBySize(size_t size);


public:
    OnlineStacker();
    virtual const RGBVector &addFrame(const uint8_t *data, size_t size, size_t w, size_t h) override final;
    virtual void setFilterQuality(size_t quality) override final; // parameter defines 2x of buffers
    virtual void reset();
};

#endif // ONLINESTUCKER_H
