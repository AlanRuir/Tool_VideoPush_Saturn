#ifndef __VIDEO_PUSH_H__
#define __VIDEO_PUSH_H__

#include <iostream>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
};

#include <common/common_type.h>

class VideoPush
{
public:
    VideoPush(const std::string& url, int width, int height, int fps, VideoCodec codec);
    ~VideoPush();
    int Push(uint8_t* data, int len);

private:
    bool StringStartWith(const std::string& s, const std::string& prefix);

private:
    std::string      url_;
    std::string      protocol_;
    AVCodec*         codec_h26x_;
    AVCodecContext*  codec_ctx_h26x_;
    AVFormatContext* context_;
    AVPacket*        av_packet_;
    AVStream*        out_stream_;
    long long        start_time_;
    size_t           frame_index_;
};

#endif  // __VIDEO_PUSH_H__