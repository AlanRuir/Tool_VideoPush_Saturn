#include <video_push.h>

VideoPush::VideoPush(uint32_t width, uint32_t height, uint32_t fps, std::string url) 
    : width_(width)
    , height_(height)
    , fps_(fps)
    , url_(url)
    , fmt_ctx_(nullptr)
{
    packet_buffer_ = std::shared_ptr<uint8_t>(new uint8_t[width * height * 3 / 2](), std::default_delete<uint8_t[]>());

    // Initialize FFmpeg
    av_log_set_level(AV_LOG_DEBUG); // Enable FFmpeg logging
    avformat_network_init();

    int ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, "rtsp", url_.c_str());
    if (ret < 0) 
    {
        std::string error = "Unable to allocate output context: " + getFFmpegError(ret);
        throw std::runtime_error(error);
    }

    if (!fmt_ctx_) 
    {
        std::string error = "Output format context is null";
        throw std::runtime_error(error);
    }

    AVOutputFormat* oformat = fmt_ctx_->oformat;

    // Create output stream
    AVStream* out_stream = avformat_new_stream(fmt_ctx_, nullptr);
    if (!out_stream) 
    {
        std::string error = "Unable to create new stream";
        avformat_free_context(fmt_ctx_);
        throw std::runtime_error(error);
    }

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    if (!codec) 
    {
        std::string error = "HEVC encoder not found";
        avformat_free_context(fmt_ctx_);
        throw std::runtime_error(error);
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) 
    {
        std::string error = "Unable to allocate codec context";
        avformat_free_context(fmt_ctx_);
        throw std::runtime_error(error);
    }

    codec_ctx->width = width_;
    codec_ctx->height = height_;
    codec_ctx->time_base = {1, fps_};  // Frame rate 30fps
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (avcodec_parameters_from_context(out_stream->codecpar, codec_ctx) < 0) 
    {
        std::string error = "Unable to copy codec parameters to output stream";
        avformat_free_context(fmt_ctx_);
        throw std::runtime_error(error);
    }

    std::cout << "Opening output URL: " << url_ << std::endl;

    av_dump_format(fmt_ctx_, 0, url_.c_str(), 1);

    // Write file header
    ret = avformat_write_header(fmt_ctx_, nullptr);
    if (ret < 0) 
    {
        std::string error = std::string("Unable to write file header, error code: ") + std::to_string(ret) + " (" + getFFmpegError(ret) + ")";
        avformat_free_context(fmt_ctx_);
        throw std::runtime_error(error);
    }

    if (!fmt_ctx_)
    {
        std::string error = "Output format context is null";
        throw std::runtime_error(error);
    }
    else
    {
        std::cout << "Output format context is valid" << std::endl;
    }
}

VideoPush::~VideoPush()
{
    // Write file trailer
    av_write_trailer(fmt_ctx_);
    avformat_free_context(fmt_ctx_);
}

std::string VideoPush::getFFmpegError(int errnum)
{
    char buf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, buf, sizeof(buf));
    return std::string(buf);
}

void VideoPush::pushFrame(const uint8_t* packet_data, uint32_t packet_size, uint64_t timestamp)
{
    (void)timestamp;
    // Create AVPacket
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = const_cast<uint8_t*>(packet_data); // Set packet data
    pkt.size = packet_size;

    if (!fmt_ctx_)
    {
        std::cerr << "Output format context is null" << std::endl;
        return;
    }

    // Write frame data to output stream
    int ret = av_interleaved_write_frame(fmt_ctx_, &pkt);
    if (ret < 0) 
    {
        std::cerr << "Failed to write frame data, error code: " << ret << " (" << getFFmpegError(ret) << ")" << std::endl;
        return;
    }
}