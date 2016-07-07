#ifndef ONLINESTUCKER_H
#define ONLINESTUCKER_H
#include <stdint.h>
#include <vector>
#include <deque>
#include <memory>

class OnlineStacker
{
public:
    using RGBVector     = std::vector<uint8_t>;
    using RGBVectorSum  = std::vector<uint32_t>;
    using QueuedElement = RGBVector;
    using RGBQueue      = std::deque<std::shared_ptr<QueuedElement>>;
private:
    RGBVectorSum   summ;
    RGBQueue       images;
    RGBVector      outImage;
    void initBySize(size_t size);


public:
    OnlineStacker();
    const RGBVector &addFrame(const RGBVector& rgb24);
    const RGBVector &addFrame(const uint8_t *data, size_t size);

    void setFilterQuality(size_t quality); // parameter defines 2x of buffers
};

#endif // ONLINESTUCKER_H
