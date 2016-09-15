#ifndef VIDEOOUTFILE_H
#define VIDEOOUTFILE_H

#include <string>
#include <memory>
#include <vector>
#include <atomic>

#include "video_inc.h"
#include "devicing/frame_listener.h"

class VideoOutFile;
using VideoOutFilePtr = std::shared_ptr<VideoOutFile>;

class VideoOutFile :public std::enable_shared_from_this<VideoOutFile>
{
public:
    class videoout_excp : public std::runtime_error
    {
    public:
        explicit videoout_excp(const std::string& t): std::runtime_error(t){}
        explicit videoout_excp(const char* t): std::runtime_error(t){}
        virtual ~videoout_excp();
    };

private:
    using AVFormatContextPtr = std::shared_ptr<AVFormatContext>;
    using AVStreamPtr        = std::shared_ptr<AVStream>;
    using AVFramePtr         = std::shared_ptr<AVFrame>;
    using FrameBuf           = std::vector<uint8_t>;

    AVFormatContextPtr       pFormatCtx;
    AVStreamPtr              pVideoStream;
    const std::string        fileName;
    const AVCodecID          encodeType;
    AVFramePtr               frame;
    FrameBuf                 picture_buf;
    std::atomic<bool>        finished;
protected:
    VideoOutFile() = delete;
    VideoOutFile(const VideoOutFile&) = delete;
    VideoOutFile(VideoOutFile&&) = delete;
    VideoOutFile& operator=(const VideoOutFile&) = delete;
    VideoOutFile& operator=(VideoOutFile&&) = delete;

    VideoOutFile(const std::string& fileName, AVCodecID encodeType);
    void closeFile();

public:
    //ensuring we will always have shared pointer, shared_from_this works
    static VideoOutFilePtr createVideoOutFile(const std::string& fileName, AVCodecID encodeType);
    virtual ~VideoOutFile();
    frame_listener_ptr createListener(uint32_t pixFormat, const std::function<void(const std::string &)> &callback_error);
    void recordDone();
    static AVPixelFormat getFormat(uint32_t fourCC);
    static void initVideoLibsOnce();
private:
    static AVFormatContextPtr newFormatContext();
    void   prepareRecording(uint32_t w, uint32_t h, uint32_t pixFormat);
    void   camera_input(__u32 w, __u32 h, const uint8_t* mem, size_t size,  int64_t ms_per_frame, uint32_t pxl_format);
};



#endif // VIDEOOUTFILE_H
