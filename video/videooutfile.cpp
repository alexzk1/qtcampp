#include "videooutfile.h"
#include <iostream>

//that ffmpeg is real mess .. google about it is even more
//really ugly code below, sorry future readers ...

extern "C"
{
const void *avpriv_get_raw_pix_fmt_tags(void);
AVPixelFormat avpriv_find_pix_fmt(const void *tags, unsigned int fourcc);
}

const static auto needFramesForFPS = 5;
#define FATAL_RISE(TEXT) throw videoout_excp(std::string(TEXT)+"\n\tat "+std::string(__FILE__)+": "+std::to_string(__LINE__))

VideoOutFile::VideoOutFile(const std::string &fileName, AVCodecID encodeType):
    pFormatCtx(nullptr),
    pVideoStream(nullptr),
    fileName(fileName),
    encodeType(encodeType),
    frame(nullptr),
    finished(false),
    img_convert_ctx(nullptr),
    summMs(0),
    framesCounted(0)
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
            WThis->recordDone();
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
    sws_freeContext(img_convert_ctx);
    img_convert_ctx = nullptr;
    summMs = 0;
    framesCounted = 0;
}

AVPixelFormat VideoOutFile::getFormat(uint32_t fourCC)
{

    //v4l and ffmpeg have different last byte for the same formats - wrong namings! I have incoming v4l
    //https://www.ffmpeg.org/doxygen/2.6/raw_8c_source.html
    switch (fourCC)
    {
        case V4L2_PIX_FMT_YUYV:
            fourCC = MKTAG('Y', 'U', 'Y', '2');
            break;
        case V4L2_PIX_FMT_YUV422P:
            fourCC = MKTAG('4', '2', '2', 'P');
            break;
        case V4L2_PIX_FMT_RGB24:
            fourCC = MKTAG('R', 'G', 'B', 24 );
            break;
        case V4L2_PIX_FMT_RGB32:
            fourCC = MKTAG('R', 'G', 'B', 32 );
            break;
        case V4L2_PIX_FMT_ARGB32:
            fourCC = MKTAG('A', 'R', 'G', 'B');
            break;
        case V4L2_PIX_FMT_GREY:
            fourCC = MKTAG('Y', '8', ' ', ' ');
        default:
            break;
    }

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
        FATAL_RISE("It seems codec is not installed.");

    pVideoStream.reset(avformat_new_stream(pFormatCtx.get(), pCodec), [this](AVStream* p)
    {
        auto pc = pFormatCtx.get();
        if (pc)
        {
            av_write_frame(pc, nullptr);//flush the data
            av_write_trailer(pc);
        }
        if (p)
            avcodec_free_context(&p->codec);
    });
    auto pCodecCtx = pVideoStream->codec;

    auto fps = static_cast<int>(1000 * needFramesForFPS / summMs) + 1;
    pVideoStream->time_base = (AVRational){ 1, fps};
    if (!pVideoStream)
    {
        avcodec_free_context(&pCodecCtx);
        FATAL_RISE("Cannot allocate video stream.");
    }


    // some formats want stream headers to be separate
    if(pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    pCodecCtx->codec_id = pOutputFormat->video_codec;


    //otherwise keep what codec wants default
    switch(encodeType)
    {
        case AV_CODEC_ID_RAWVIDEO:
            pCodecCtx->pix_fmt  = getFormat(pixFormat);
            break;
        case AV_CODEC_ID_MJPEG:
            pCodecCtx->pix_fmt  = AV_PIX_FMT_YUVJ422P;
            break;
        case AV_CODEC_ID_HUFFYUV:
        case AV_CODEC_ID_PNG:
            pCodecCtx->pix_fmt  = AV_PIX_FMT_RGB24;
            break;
        default:
            pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
            break;
    }

    pCodecCtx->width    = static_cast<int>(w);
    pCodecCtx->height   = static_cast<int>(h);
    pCodecCtx->bit_rate = 10000000;
    pCodecCtx->gop_size = 12;
    pCodecCtx->time_base = pVideoStream->time_base;//(AVRational){ 1, 60};

    if(pOutputFormat->video_codec == AV_CODEC_ID_H264 || pOutputFormat->video_codec == AV_CODEC_ID_VP8)
    {
        pCodecCtx->qmin = 15; // qmin = 10*
        pCodecCtx->qmax = 30; //qmax = 51 **
    }

    int r = avcodec_open2(pCodecCtx, pCodec, NULL);

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
        frame->format = pCodecCtx->pix_fmt;

        if (!(pOutputFormat->flags & AVFMT_NOFILE)) {
            if (avio_open(&pFormatCtx->pb, fileName.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                FATAL_RISE("Could not create file "+fileName);
            }
        }
        int res = avformat_write_header(pFormatCtx.get(),NULL);
        if (res < 0)
        {
            FATAL_RISE("Failed to write format header with code " + std::to_string(res));
        }
    }

    if (r < 0)
    {
        char buf[2048];
        av_make_error_string(buf, 2048, r);
        FATAL_RISE("Failed to open codec, possibly pixel format is not supported: " + std::string(buf));
    }
}

void VideoOutFile::camera_input(__u32 w, __u32 h, const uint8_t *mem, size_t size, int64_t ms_per_frame, uint32_t pxl_format)
{
     //ok, most players need step +1, so thats why need to have FPS prior starting
    //getting some avreege
    summMs += ms_per_frame;
    if (framesCounted++ < needFramesForFPS)
        return;

    if (!finished)
    {
        if (!pVideoStream)
        {
            prepareRecording(w, h, pxl_format);

            frame->width  = w;
            frame->height = h;
            frame->pts    = 0;

        }
        auto pCodecCtx=pVideoStream->codec;
        auto effective_sz = std::min(size, picture_buf.size());
        if(AV_CODEC_ID_RAWVIDEO == encodeType)
            memcpy(picture_buf.data(), mem, effective_sz);
        else
        {
            //need to make conversion to codec's pixel format
            auto pxf = getFormat(pxl_format);
            img_convert_ctx = sws_getCachedContext(img_convert_ctx, w, h, pxf, w, h, pCodecCtx->pix_fmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
            if (!img_convert_ctx)
            {
                std::cerr << "Failed to create converter"<<std::endl;
                return;
            }

            uint8_t *srcplanes[3];
            srcplanes[0]= const_cast<uint8_t*>(mem);
            srcplanes[1]=0;
            srcplanes[2]=0;

            auto bytes_per_pixel = size / (w*h);
            int srcstride[3];
            srcstride[0]=bytes_per_pixel * w;
            srcstride[1]=0;
            srcstride[2]=0;

            sws_scale(img_convert_ctx, srcplanes, srcstride, 0, h, frame->data, frame->linesize);
        }

        int got_packet = 0;
        int out_size = 0;
        AVPacket pkt;


        pkt.data = NULL;
        pkt.size = 0;
        av_init_packet(&pkt);
        pkt.dts = AV_NOPTS_VALUE;
        pkt.pts = frame->pts;
        pkt.duration = ms_per_frame;

        /* encode the image */
        int ret = avcodec_encode_video2(pCodecCtx, &pkt, frame.get(), &got_packet);
        //avcodec_encode_video2(pCodecCtx, &pkt, nullptr, &got_packet);
        //avcodec_send_frame()
        if (ret > -1 && got_packet)
        {
            //if (pCodecCtx->coded_frame->pts != AV_NOPTS_VALUE)
            //  pkt.pts= av_rescale_q(pCodecCtx->coded_frame->pts, pCodecCtx->time_base, pVideoStream->time_base);
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
            av_packet_unref(&pkt);
        }
        frame->pts += 1;//ms_per_frame;
    }
}

VideoOutFile::videoout_excp::~videoout_excp(){}
