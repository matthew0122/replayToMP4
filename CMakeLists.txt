cmake_minimum_required(VERSION 3.16)
project(TagPro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Box2D (from source) ----
add_subdirectory(extern/box2d)

# ---- FFmpeg paths ----
set(FFMPEG_ROOT "C:/ffmpeg/ffmpeg-8.0.1-full_build-shared")

include_directories(${FFMPEG_ROOT}/include)
link_directories(${FFMPEG_ROOT}/lib)

# ---- Executable ----
add_executable(tagpro
    tagpro_renderer.cpp
    physics.cpp
)

# ---- Link libraries ----
target_link_libraries(tagpro PRIVATE
    box2d
    avformat
    avcodec
    avutil
    swscale
    ws2_32
    secur32
    bcrypt
)
