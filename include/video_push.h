#ifndef __VIDEO_PUSH_H__
#define __VIDEO_PUSH_H__

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <cstdint>
#include <fstream>

extern "C" 
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

class VideoPush
{
public:
    VideoPush(uint32_t width, uint32_t height, uint32_t fps, std::string url);
    ~VideoPush();
    void pushFrame(const uint8_t* packet_data, uint32_t packet_size, uint64_t timestamp);

private:
    std::string getFFmpegError(int errnum);

private:
    uint32_t                 width_;
    uint32_t                 height_;
    int                      fps_;
    std::string              url_;
    AVFormatContext*         fmt_ctx_;
    std::shared_ptr<uint8_t> packet_buffer_;
};

#endif  // __VIDEO_PUSH_H__