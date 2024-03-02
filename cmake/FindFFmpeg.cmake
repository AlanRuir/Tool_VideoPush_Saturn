# 设置FFmpeg库的根目录
set(FFmpeg_ROOT_DIR "/usr")  # ！！！修改为你的FFmpeg库的根目录

# 设置FFmpeg库的头文件和库文件路径
set(FFmpeg_INCLUDE_DIR "${FFmpeg_ROOT_DIR}/include")
set(FFmpeg_LIBRARY_DIR "${FFmpeg_ROOT_DIR}/lib/x86_64-linux-gnu")

# 设置FFmpeg库的可执行文件路径（如果需要）
set(FFmpeg_EXECUTABLE_DIR "${FFmpeg_ROOT_DIR}/bin")

# 设置FFmpeg库的具体库文件名
set(FFmpeg_LIBRARIES
    "${FFmpeg_LIBRARY_DIR}/libavcodec.so"
    "${FFmpeg_LIBRARY_DIR}/libavformat.so"
    "${FFmpeg_LIBRARY_DIR}/libavfilter.so"
    "${FFmpeg_LIBRARY_DIR}/libavutil.so"
    # 添加其他需要的库文件
)

# 设置其他编译选项（如果需要）
set(FFmpeg_COMPILE_FLAGS "-DENABLE_FEATURE_X")

# 导出上述变量和选项
set(FFmpeg_FOUND TRUE)