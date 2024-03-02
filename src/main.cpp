#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <cstdint>
#include <unistd.h>
#include <video/video_push.h>

uint64_t GetFileSize(const std::string& filePath)       // 获取文件大小
{
    std::ifstream file(filePath, std::ifstream::ate | std::ifstream::binary);
    if (!file) 
    {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return 0;
    }

    return static_cast<uint64_t>(file.tellg());
}

void DisplayProgress(int current, int total)        // 显示进度
{
    const int barWidth = 70;
    float progress = static_cast<float>(current) / total;
    int barLength = static_cast<int>(progress * barWidth);

    std::string progressBar = "[";
    for (int i = 0; i < barLength; ++i) 
    {
        progressBar += "=";
    }
    for (int i = barLength; i < barWidth; ++i) 
    {
        progressBar += " ";
    }
    progressBar += "]";

    std::string progressLog = "Progress: " + std::to_string(current) + "/" + std::to_string(total);

    std::cout << "\r" << progressBar << " " << progressLog << std::flush;
}

int main(int argc, char* argv[])
{
    FILE* file = nullptr;
    uint32_t frame_size = 1920 * 1080 * 3 / 2;
    std::shared_ptr<uint8_t> frame_buffer(new uint8_t[frame_size](), std::default_delete<uint8_t[]>());

    std::shared_ptr<VideoPush> video_push = std::make_shared<VideoPush>("rtsp://10.1.0.127:19093/live/av/0", 1920, 1080, 30, VideoCodec::H265);

    std::cout << "开始推送" << std::endl;
    for (int j = 1; j <= 500; ++j)
    {
        for (int i = 1; i <= 500; ++i)
        {
            std::string file_path = std::string("../shell/output/") + std::to_string(i) + ".265";
            file = fopen(file_path.c_str(), "rb");                      // 打开文件
            uint64_t file_size = GetFileSize(file_path);                // 获取文件大小
            fread(frame_buffer.get(), 1, file_size, file);              // 从文件中读取编码数据
            video_push->Push(frame_buffer.get(), frame_size);  // 推送编码数据
            usleep(1000 * 100);
            fclose(file);                                               // 关闭文件
            DisplayProgress(i, 500);                                    // 显示进度
        }
    }
    std::cout << std::endl;

    return 0;
}