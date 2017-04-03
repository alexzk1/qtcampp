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

const OnlineStacker::RGBVector &OnlineStacker::addFrame(const uint8_t *data, size_t size, size_t w, size_t h)
{
    (void)w;
    (void)h;

    initBySize(size);
    auto f = images.front();

    auto div = images.size();
    for (size_t i  = 0; i < size; ++i)
    {
        summ[i]     = summ[i] - f->at(i) + *(data + i);
        outImage[i] = static_cast<uint8_t>(summ[i] / div);
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

void OnlineStacker::reset()
{
    outImage.clear();
    summ.clear();
}

void OnlineStacker::initBySize(size_t size)
{
    if (!images.front() || images.front()->size() != size)
    {
        reset();
        outImage.resize(size, 0);
        summ.resize(size, 0);

        for (auto& v: images)
        {
            //http://stackoverflow.com/questions/20895648/difference-in-make-shared-and-normal-shared-ptr-in-c
            v = std::shared_ptr<QueuedElement>(new QueuedElement());
            v->resize(size, 0);
        }
    }
}

