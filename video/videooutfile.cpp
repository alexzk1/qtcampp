#include "videooutfile.h"

//that ffmpeg is real mess .. google about it is even more

extern "C"
{
const void *avpriv_get_raw_pix_fmt_tags(void);
AVPixelFormat avpriv_find_pix_fmt(const void *tags, unsigned int fourcc);
}

#define FATAL_RISE(TEXT) throw videoout_excp(std::string(TEXT)+"\n\tat "+std::string(__FILE__)+": "+std::to_string(__LINE__))

VideoOutFile::VideoOutFile(const std::string &fileName, AVCodecID encodeType):
    pFormatCtx(nullptr),
    pVideoStream(nullptr),
    fileName(fileName),
    encodeType(encodeType),
    frame(nullptr),
    finished(false)
{
}

VideoOutFilePtr VideoOutFile::createVideoOutFile(const std::string &fileName, AVCodecID encodeType)
{
    return VideoOutFilePtr(new VideoOutFile(fileName, encodeType));
}

VideoOutFile::~VideoOutFile()
{
    closeFile();
}

frame_listener_ptr VideoOutFile::createListener(uint32_t pixFormat, const std::function<void(const std::string&)>& callback_error)
{
    //this lambda locks shared pointer, so until listener exists this object will exists too
    auto WThis = shared_from_this();
    frame_listener_ptr listener(new frame_listener([WThis, callback_error](__u32 w, __u32 h, const uint8_t *mem, size_t size,
                                                   int64_t ms_per_frame, uint32_t pxl_format)
    {
        try
        {
            WThis->camera_input(w, h, mem, size, ms_per_frame, pxl_format);
        } catch (videoout_excp& e)
        {
            callback_error(e.what());
        }

    }, pixFormat));
    return listener;
}

void VideoOutFile::recordDone()
{
    finished = true;
}

void VideoOutFile::closeFile()
{
    pVideoStream.reset();
    frame.reset();
    pFormatCtx.reset();
}

AVPixelFormat VideoOutFile::getFormat(uint32_t fourCC)
{
    //I need back conversion - we have device listener with fourCC code set, need same AV format
    return avpriv_find_pix_fmt(avpriv_get_raw_pix_fmt_tags(), fourCC);
}

void VideoOutFile::initVideoLibsOnce()
{
    //should be called from the main()
    av_register_all();
    avcodec_register_all();
}


VideoOutFile::AVFormatContextPtr VideoOutFile::newFormatContext()
{
    AVFormatContextPtr res(avformat_alloc_context(), [](AVFormatContext* p)
    {
        if (p)
        {
            //p->oformat        = nullptr;
            /* free the streams */

            for(unsigned int i = 0; i < p->nb_streams; i++)
            {
                av_freep(&p->streams[i]->codec);
                av_freep(&p->streams[i]);
            }
            // Close file
            avio_close(p->pb);
            av_free(p);
            //avformat_free_context(p);
        }
    });
    return res;
}

void VideoOutFile::prepareRecording(uint32_t w, uint32_t h, uint32_t pixFormat)
{
    pFormatCtx = newFormatContext();
    //AV_CODEC_ID_RAWVIDEO
    AVOutputFormat * pOutputFormat = av_guess_format(nullptr, fileName.c_str(), nullptr);
    if (!pOutputFormat) {
        pOutputFormat = av_guess_format("mpeg", nullptr, nullptr);
    }

    if (!pOutputFormat)
        FATAL_RISE("Cannot find installed output format (container).");

    //pOutputFormat->video_codec = static_cast<AVCodecID>(encodeType); //must not be done, since that format seems global static
    if(pOutputFormat->video_codec == AV_CODEC_ID_NONE)
        FATAL_RISE("Could not find proper codec.");

    pFormatCtx->oformat        = pOutputFormat;
    pOutputFormat->video_codec = encodeType;
    snprintf(pFormatCtx->filename, sizeof(pFormatCtx->filename), "%s", fileName.c_str());

    auto pCodec = avcodec_find_encoder(pOutputFormat->video_codec);
    if (!pCodec)
        FATAL_RISE("Was not able to find encoder object.");

    pVideoStream.reset(avformat_new_stream(pFormatCtx.get(), pCodec), [this](AVStream* p)
    {
        if (pFormatCtx)
        {
            av_write_frame(pFormatCtx.get(), NULL);//flush the data
            av_write_trailer(pFormatCtx.get());
        }
        if (p)
            avcodec_free_context(&p->codec);
    });
    auto pCodecCtx = pVideoStream->codec;

    pVideoStream->time_base = (AVRational){ 1, 1000};
    if (!pVideoStream)
    {
        avcodec_free_context(&pCodecCtx);
        FATAL_RISE("Cannot allocate video stream.");
    }


    // some formats want stream headers to be separate
    if(pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    pCodecCtx->codec_id = pOutputFormat->video_codec;
    pCodecCtx->pix_fmt  = getFormat(pixFormat);
    pCodecCtx->width    = static_cast<int>(w);
    pCodecCtx->height   = static_cast<int>(h);
    pCodecCtx->bit_rate = 10000000;
    pCodecCtx->gop_size = 12;
    pCodecCtx->time_base = (AVRational){ 1, 60};

    if(pOutputFormat->video_codec == AV_CODEC_ID_H264 || pOutputFormat->video_codec == AV_CODEC_ID_VP8)
    {
        pCodecCtx->qmin = 15; // qmin = 10*
        pCodecCtx->qmax = 30; //qmax = 51 **
    }

    int r = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (r < 0)
    {
        char buf[2048];
        av_make_error_string(buf, 2048, r);
        FATAL_RISE("Failed to open codec, code: " + std::string(buf));
    }


    auto size        = av_image_get_buffer_size(pCodecCtx->pix_fmt, static_cast<int>(w), static_cast<int>(h), 1);
    picture_buf.resize(static_cast<size_t>(size), 0);

    if (picture_buf.size())
    {
        frame.reset(av_frame_alloc(), [](AVFrame* p)
        {
            if (p)
                av_frame_free(&p);
        });
        av_image_fill_arrays(frame->data, frame->linesize, picture_buf.data(), pCodecCtx->pix_fmt, static_cast<int>(w), static_cast<int>(h), 1);
        if (!(pOutputFormat->flags & AVFMT_NOFILE)) {
            if (avio_open(&pFormatCtx->pb, fileName.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                closeFile();
                FATAL_RISE("Could not create file "+fileName);
            }
        }
        int res = avformat_write_header(pFormatCtx.get(),NULL);
        if (res < 0)
        {
            closeFile();
            FATAL_RISE("Failed to write format header with code " + std::to_string(res));
        }
    }
}

void VideoOutFile::camera_input(__u32 w, __u32 h, const uint8_t *mem, size_t size, int64_t ms_per_frame, uint32_t pxl_format)
{
    if (!finished)
    {
        if (!pVideoStream)
        {
            prepareRecording(w, h, pxl_format);
            frame->format = getFormat(pxl_format);
            frame->width  = w;
            frame->height = h;
            frame->pts = 0;
        }
        frame->pts += ms_per_frame;
        auto effective_sz = std::min(size, picture_buf.size());
        memcpy(picture_buf.data(), mem, effective_sz);

        int got_packet = 0;
        int out_size = 0;
        AVPacket pkt;
        auto pCodecCtx=pVideoStream->codec;

        pkt.data = NULL;
        pkt.size = 0;
        av_init_packet(&pkt);
        pkt.pts = pkt.dts = frame->pts;

        /* encode the image */
        int ret = avcodec_encode_video2(pCodecCtx, &pkt, frame.get(), &got_packet);
        if (ret > -1 && got_packet)
        {
            if (pCodecCtx->coded_frame->pts != AV_NOPTS_VALUE)
                pkt.pts= av_rescale_q(pCodecCtx->coded_frame->pts, pCodecCtx->time_base, pVideoStream->time_base);
            pkt.stream_index = pVideoStream->index;
            //        if((tempExtensionCheck) == "mkv") {
            //            i++;
            //            pkt.pts = (i*(1000/pCodecCtx->gop_size));
            //            pkt.dts = pkt.pts;
            //        }
            if(pCodecCtx->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;

            /* Write the compressed frame to the media file. */
            out_size = av_write_frame(pFormatCtx.get(), &pkt);
            av_free_packet(&pkt);
        }
    }
    else
        closeFile();
}

VideoOutFile::videoout_excp::~videoout_excp(){}
