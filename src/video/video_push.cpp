#include <memory>
#include <functional>
#include <video/video_push.h>

VideoPush::VideoPush(const std::string& url, int width, int height, int fps, VideoCodec codec)
    : url_(url)
    , protocol_("tcp")
    , codec_h26x_(nullptr)
    , codec_ctx_h26x_(nullptr)
    , context_(nullptr)
    , av_packet_(nullptr)
    , out_stream_(nullptr)
    , start_time_(0)
    , frame_index_(0U)
{
    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    if (VideoCodec::H264 == codec)
    {
        codec_h26x_ = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    else
    {
        codec_h26x_ = avcodec_find_encoder(AV_CODEC_ID_H265);
    }
    if (!codec_h26x_)
    {
        std::cout << "Fail: avcodec_find_encoder" << std::endl;
    }

    codec_ctx_h26x_ = avcodec_alloc_context3(codec_h26x_);
    codec_ctx_h26x_->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx_h26x_->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx_h26x_->width = width;
    codec_ctx_h26x_->height = height;
    codec_ctx_h26x_->channels = 3;
    codec_ctx_h26x_->time_base = {1, fps}; // 设置帧率
    codec_ctx_h26x_->gop_size = fps;  // 设置关键帧间隔
    codec_ctx_h26x_->max_b_frames = 0;
    codec_ctx_h26x_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    std::string format_name;
    if (StringStartWith(url_, "rtsp://")) 
    {
        format_name = "rtsp";
    }else if(StringStartWith(url_, "udp://") || StringStartWith(url_, "tcp://")) 
    {
        format_name = "h264";
    }else if(StringStartWith(url_, "rtp://")) 
    {
        format_name = "rtp";
    }else if(StringStartWith(url_, "rtmp://")) 
    {
        format_name = "flv";
    }
    else
    {
        throw std::runtime_error("Not support this Url");
    }  

    if (!context_)
    {
        int ret = avformat_alloc_output_context2(&context_, NULL, format_name.c_str(), url_.c_str()); 
        if (ret < 0 || !context_) 
        {
            std::cout << "error: " << ret << std::endl;
            throw std::runtime_error("Failed to allocate output context");
        }

        av_opt_set(context_->priv_data, "rtsp_transport", protocol_.c_str(), 0);
        context_->start_time_realtime = av_gettime(); // 设置起始时间戳

        out_stream_ = avformat_new_stream(context_, codec_h26x_);
        if (!out_stream_) 
        {
            std::cout << "outStream failed " << std::endl;
            throw std::runtime_error("Failed to create new stream");
        }

        out_stream_->time_base = {1, fps}; // 设置帧率

        ret = avcodec_parameters_from_context(out_stream_->codecpar, codec_ctx_h26x_);
        if (ret < 0)
        {
            std::cout << "Failed to copy codec parameters" << std::endl;
            throw std::runtime_error("Failed to copy codec parameters");
        }

        context_->oformat->flags |= AVFMT_TS_NONSTRICT;
        if (context_->oformat->flags & AVFMT_GLOBALHEADER) 
        {
            out_stream_->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
    }

    av_dump_format(context_, 0, url_.c_str(), 1);
    
    int ret = avio_open(&context_->pb, url_.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) 
    {
        std::cout << "avio_open failed" << std::endl;
        throw std::runtime_error("Failed to open output URL");
    }

    ret = avformat_write_header(context_, NULL);
    if ( ret < 0) 
    {
        std::cout << "avformat_write_header failed" << std::endl;
        throw std::runtime_error("Failed to write output header");
    }

    start_time_ = av_gettime();
}

VideoPush::~VideoPush()
{
    avcodec_close(codec_ctx_h26x_);
    avcodec_free_context(&codec_ctx_h26x_);
    avformat_free_context(context_);
    av_packet_free(&av_packet_);
}

int VideoPush::Push(uint8_t* data, int len)
{
    av_packet_unref(av_packet_); // 清空 packet

    // 计算时间戳
    int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(out_stream_->time_base);
    av_packet_->pts = (double)(frame_index_ * calc_duration) / (double)(av_q2d(out_stream_->time_base) * AV_TIME_BASE);
    av_packet_->dts = av_packet_->pts;
    av_packet_->duration = (double)calc_duration / (double)(av_q2d(out_stream_->time_base) * AV_TIME_BASE);

    // 设置 packet 数据
    av_packet_->size = len;
    av_packet_->data = data; 
    
    // 时间戳转换
    av_packet_->pts = av_rescale_q_rnd(av_packet_->pts, out_stream_->time_base, out_stream_->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    av_packet_->dts = av_rescale_q_rnd(av_packet_->dts, out_stream_->time_base, out_stream_->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    av_packet_->duration = av_rescale_q_rnd(av_packet_->duration, out_stream_->time_base, out_stream_->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    av_packet_->pos = -1;

    // 写入一帧数据到输出流
    int ret = av_interleaved_write_frame(context_, av_packet_);
    if (ret < 0) 
    {
        std::cout << "av_interleaved_write_frame failed" << std::endl;
    }

    ++frame_index_;

    return ret;
}

bool VideoPush::StringStartWith(const std::string& s, const std::string& prefix)
{
    return (s.compare(0, prefix.size(), prefix) == 0);
}
