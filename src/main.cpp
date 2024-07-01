#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <cstdint>
#include <fstream>
#include <csignal>

extern "C" 
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

#include <video_push.h>

const int FRAME_WIDTH = 1920;
const int FRAME_HEIGHT = 760;
const int FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3 / 2;  // YUV420 frame size
const char* RTSP_URL = "rtsp://127.0.0.1:554/live/test";
const int FRAME_INTERVAL_MS = 40;  // milliseconds
std::shared_ptr<uint8_t> packet_buffer(new uint8_t[FRAME_SIZE](), std::default_delete<uint8_t[]>());

uint64_t GetFileSize(const std::string& filePath) 
{
    std::ifstream file(filePath, std::ifstream::ate | std::ifstream::binary);
    if (!file) 
    {
        std::cerr << "Unable to open file: " << filePath << std::endl;
        return 0;
    }
    return static_cast<uint64_t>(file.tellg());
}

void SignalHandler(int sig)
{
    struct sigaction new_action;
    new_action.sa_handler = SIG_DFL;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction(sig, &new_action, NULL);
    raise(sig);
}

int main() 
{
    struct sigaction action;
    action.sa_handler = SignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    
    std::shared_ptr<VideoPush> video_push = std::make_shared<VideoPush>(FRAME_WIDTH, FRAME_HEIGHT, 25, RTSP_URL);

    for (int i = 0; i < 10; ++i)
    {
        for (size_t i = 1; i < 512; ++i) 
        {
            auto start = std::chrono::high_resolution_clock::now();

            // Read frame data from disk
            std::string file_path = "../packets/" + std::to_string(i) + ".265";
            FILE* file = fopen(file_path.c_str(), "rb");
            if (!file) 
            {
                std::cerr << "Unable to open file: " << file_path << std::endl;
                continue;
            }
            uint64_t file_size = GetFileSize(file_path);
            fread(packet_buffer.get(), 1, file_size, file);

            video_push->pushFrame(packet_buffer.get(), static_cast<uint32_t>(file_size), static_cast<uint64_t>(i));
            
            // Control frame rate
            auto end = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_INTERVAL_MS) - std::chrono::duration_cast<std::chrono::milliseconds>(end - start));
            fclose(file);
        }
    }

    return 0;
}
