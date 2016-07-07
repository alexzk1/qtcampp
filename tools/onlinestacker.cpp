#include "onlinestacker.h"
#include <memory.h>
#include <algorithm>
#include <math.h>

OnlineStacker::OnlineStacker():
    summ(),
    images(),
    outImage()
{
    setFilterQuality(1);
}

const OnlineStacker::RGBVector & OnlineStacker::addFrame(const OnlineStacker::RGBVector &rgb24)
{
    return addFrame(rgb24.data(), rgb24.size());
}

const OnlineStacker::RGBVector &OnlineStacker::addFrame(const uint8_t *data, size_t size)
{
    initBySize(size);
    auto f = images.front();

    for (size_t i  = 0; i < size; ++i)
    {
        summ[i]     = summ[i] - f->at(i) + *(data + i);
        outImage[i] = static_cast<uint8_t>(summ[i] / images.size());
    }

    memcpy(f->data(), data, size);

    images.pop_front();
    images.push_back(f);


    return outImage;
}

void OnlineStacker::setFilterQuality(size_t quality)
{
    size_t pow = 1 << std::max<size_t>(quality, 1);
    if (images.size() != pow)
    {
        images.clear();
        images.resize(pow);

        auto sz = summ.size();
        summ.clear();
        summ.resize(sz, 0);
    }
}

void OnlineStacker::initBySize(size_t size)
{

    if (!images.front() || images.front()->size() != size)
    {
        for (auto& v: images)
        {
            v = std::make_shared<QueuedElement>();
            v->resize(size, 0);
        }
        outImage.clear();
        outImage.resize(size, 0);

        summ.clear();
        summ.resize(size, 0);
    }
}

