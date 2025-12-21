#define __STDC_CONSTANT_MACROS
//puv constants
#define WR 0.299
#define WB 0.114
#define WG 0.587
#define UMAX 0.436
#define VMAX 0.615
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include "nlohmann/json.hpp"


using json = nlohmann::json;

std::vector<std::vector<std::string>> getMap(std::string filename);
std::vector<double> RGB_to_PUV(int r, int g, int b);

int main() {
    const char* filename = "output.mp4";
    // const int width = 640;
    // const int height = 480;
    const int fps = 60;
    const int duration_sec = 5;
    const int tile_size = 40;
    const std::string replayFile = "replay.ndjson";

    std::vector<std::vector<std::string>> map = getMap(replayFile);
    const int width = tile_size * map.size();
    const int height = tile_size * map[0].size();

    std::cout << width << std::endl;
    std::cout << height << std::endl;

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
    stream->time_base = {1, fps}; 

    // Allocate codec context
    AVCodecContext* cctx = avcodec_alloc_context3(codec);
    cctx->codec_id = codec->id;
    cctx->width = width;
    cctx->height = height;
    cctx->time_base = {1, fps};  // 1/fps seconds per frame
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

    AVPacket* pkt = av_packet_alloc();  // Use av_packet_alloc instead

    std::vector<double> puv = RGB_to_PUV(256,5,5);

    for (int i = 0; i < fps * duration_sec; ++i) {
        av_frame_make_writable(frame);
        frame->pts = i; 

        // Simple color pattern - cycles through brightness
        int brightness = (i * 255) / (fps * duration_sec);
        memset(frame->data[0], brightness, width * height);      // Y
        memset(frame->data[1], puv[1], width * height / 4);         // U
        memset(frame->data[2], puv[2], width * height / 4);         // V

        if (avcodec_send_frame(cctx, frame) < 0) {
            std::cerr << "Error sending frame\n";
            return -1;
        }

        while (avcodec_receive_packet(cctx, pkt) == 0) {
            // Rescale packet timestamps from codec time_base to stream time_base
            av_packet_rescale_ts(pkt, cctx->time_base, stream->time_base);
            pkt->stream_index = stream->index;
            
            if (av_interleaved_write_frame(fmt_ctx, pkt) < 0) {
                std::cerr << "Error writing frame\n";
            }
            av_packet_unref(pkt);
        }
    }

    // Flush encoder
    avcodec_send_frame(cctx, nullptr);
    while (avcodec_receive_packet(cctx, pkt) == 0) {
        av_packet_rescale_ts(pkt, cctx->time_base, stream->time_base);
        pkt->stream_index = stream->index;
        av_interleaved_write_frame(fmt_ctx, pkt);
        av_packet_unref(pkt);
    }

    // Write trailer
    av_write_trailer(fmt_ctx);

    // Clean up
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&cctx);
    avio_close(fmt_ctx->pb);
    avformat_free_context(fmt_ctx);

    std::cout << "MP4 written to output.mp4 (" << duration_sec << " seconds at " << fps << " fps)\n";

    getMap("replay.ndjson");
    return 0;
}

std::vector<std::vector<std::string>> getMap(std::string filename){
    std::ifstream file(filename);
    std::string str; 
    std::vector<std::vector<std::string>> map;
    json j;
    bool inElement = false;
    while(std::getline(file, str)){
        if(str.rfind("[0,\"map\",{\"tiles\":", 0) == 0){
            j = json::parse(str);
            break;
        }
    }
    auto& tiles =  j[2]["tiles"];
    for (const auto& row : tiles){
        std::vector<std::string> rowVec;
        for (const auto& cell : row){
            if(cell.is_string()){
                rowVec.push_back(cell.get<std::string>());
            }
            else {
                rowVec.push_back(cell.dump());
            }

        }
        map.push_back(rowVec);
    }
    return map;
}

std::vector<double> RGB_to_PUV(int R, int G, int B){
    double Y = WR * R + WG * G + WB * B;
    double U = UMAX * (B-Y)/(1-WB);
    double V = VMAX * (R-Y)/(1-WR);

    return {Y, U, V};
}   



