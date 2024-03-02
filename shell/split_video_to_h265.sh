#!/bin/bash
# 将一个mp4文件拆分为多个yuv420p文件

# 获取输入的MP4文件路径
mp4_file="$1"

# 检查输入参数是否为空
if [ -z "$mp4_file" ]; then
  echo "请输入一个MP4文件路径作为参数"
  exit 1
fi

output_path="output/"
mkdir -p "$output_path"

frame_index=1

# 获取总帧数
total_frames=$(ffprobe -v error -count_frames -select_streams v:0 -show_entries stream=nb_read_frames -of default=nokey=1:noprint_wrappers=1 "$mp4_file")

# 输出开始拆分的信息
echo "开始拆分MP4文件：$mp4_file"

# 检查是否有CUDA支持
if nvidia-smi &> /dev/null; then
    echo "检测到CUDA支持，将使用CUDA加速"
    hwaccel="cuvid"
else
    echo "未检测到CUDA支持，将使用默认加速"
    hwaccel="auto"
fi

# 开始拆分
while true; do
    # 使用ffmpeg命令提取指定帧的YUV420P格式，并保存为相应的文件
    ffmpeg -y -hide_banner -loglevel panic -hwaccel "$hwaccel" -i "$mp4_file" -vf "select='eq(n,$frame_index)',format=yuv420p" -vframes 1 -vsync vfr "$output_path/$frame_index.265"

    # 检查是否已提取完所有帧
    if [ $frame_index -eq $((total_frames + 1)) ]; then
        break
    fi

    frame_index=$((frame_index + 1))

    # 计算进度百分比
    progress=$((frame_index * 100 / total_frames))

    # 计算进度条长度
    progress_bar_width=50
    progress_bar_filled=$((progress * progress_bar_width / 100))

    # 显示进度条
    echo -ne "拆分进度：["
    for ((i=0; i<progress_bar_width; i++)); do
        if ((i <= progress_bar_filled)); then
            echo -ne "="
        else
            echo -ne " "
        fi
    done
    echo -ne ">$progress%]\r"

    # 等待一段时间，以避免刷新过快
    sleep 0.1
done

echo -e "\n拆分完成！共提取了 $total_frames 帧"