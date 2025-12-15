#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <iostream>

int main() {
    std::cout << "FFmpeg version: " << av_version_info() << std::endl;
    return 0;
}
