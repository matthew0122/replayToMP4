#define __STDC_CONSTANT_MACROS
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <cstring>

int main() {
    const char* filename = "output.mp4";
    const int width = 640;
    const int height = 480;
    const int fps = 30;
    const int duration_sec = 5;

    avformat_network_init();

    // Allocate output context
    AVFormatContext* fmt_ctx = nullptr;
    avformat_alloc_output_context2(&fmt_ctx, nullptr, "mp4", filename);
    if (!fmt_ctx) {
        std::cerr << "Could not allocate output context\n";
        return -1;
    }

    // Find H.264 encoder
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Codec not found\n";
        return -1;
    }

    // Create new stream
    AVStream* stream = avformat_new_stream(fmt_ctx, codec);
    stream->id = 0;

    // Allocate codec context
    AVCodecContext* cctx = avcodec_alloc_context3(codec);
    cctx->codec_id = codec->id;
    cctx->width = width;
    cctx->height = height;
    cctx->time_base = {1, fps};
    cctx->framerate = {fps, 1};
    cctx->gop_size = 12;
    cctx->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(cctx->priv_data, "preset", "ultrafast", 0);

    if (avcodec_open2(cctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec\n";
        return -1;
    }

    avcodec_parameters_from_context(stream->codecpar, cctx);

    // Open output file
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file\n";
            return -1;
        }
    }

    avformat_write_header(fmt_ctx, nullptr);

    // Allocate frame
    AVFrame* frame = av_frame_alloc();
    frame->format = cctx->pix_fmt;
    frame->width = width;
    frame->height = height;
    av_frame_get_buffer(frame, 32);

    AVPacket pkt;
    av_init_packet(&pkt);

    for (int i = 0; i < fps * duration_sec; ++i) {
        av_frame_make_writable(frame);
        frame->pts = i;

        // Simple color pattern
        memset(frame->data[0], i * 2, width * height);           // Y
        memset(frame->data[1], 128, width * height / 4);         // U
        memset(frame->data[2], 128, width * height / 4);         // V

        if (avcodec_send_frame(cctx, frame) < 0) {
            std::cerr << "Error sending frame\n";
            return -1;
        }

        while (avcodec_receive_packet(cctx, &pkt) == 0) {
            av_interleaved_write_frame(fmt_ctx, &pkt);
            av_packet_unref(&pkt);
        }
    }

    // Flush encoder
    avcodec_send_frame(cctx, nullptr);
    while (avcodec_receive_packet(cctx, &pkt) == 0) {
        av_interleaved_write_frame(fmt_ctx, &pkt);
        av_packet_unref(&pkt);
    }

    // Write trailer
    av_write_trailer(fmt_ctx);

    // Clean up
    av_frame_free(&frame);
    avcodec_free_context(&cctx);
    avio_close(fmt_ctx->pb);
    avformat_free_context(fmt_ctx);

    std::cout << "MP4 written to output.mp4\n";
    return 0;
}
