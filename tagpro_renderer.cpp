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
void rgb_to_yuv420p(uint8_t* rgb_data, int width, int height, AVFrame* yuv_frame);
void drawWall(int x, int y, std::vector<uint8_t>*rgb, int width);
void draw1_1(int x, int y, std::vector<uint8_t> *rgb, int width);
void draw1_2(int x, int y, std::vector<uint8_t> *rgb, int width);
void draw1_3(int x, int y, std::vector<uint8_t> *rgb, int width);
void draw1_4(int x, int y, std::vector<uint8_t> *rgb, int width);
void drawFloor(int x, int y, std::vector<uint8_t> *rgb, int width);

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
    std::vector<uint8_t> rgb(width * height * 3);

    for (int i = 0; i < fps * duration_sec; ++i) {
        
        av_frame_make_writable(frame);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int j = (y * width + x) * 3;

                rgb[j + 0] = x % 256;      // R
                rgb[j + 1] = y % 256;      // G
                rgb[j + 2] = 128;          // B
            }
        }
        for(int i = 0; i < map.size(); i++){
            for(int j = 0; j < map[i].size(); j++){
                std::string tile = map[i][j];
                if(tile == "1"){
                    drawWall(i,j,&rgb, width);
                }
                else if (tile == "1.1"){
                    draw1_1(i,j, &rgb, width);
                }
                else if (tile == "1.2"){
                    draw1_2(i,j, &rgb, width);
                }
                else if (tile == "1.3"){
                    draw1_3(i,j, &rgb, width);
                }
                else if (tile == "1.4"){
                    draw1_4(i,j, &rgb, width);
                }
                else if (tile == "2"){
                    drawFloor(i,j,&rgb, width);
                }
            }
        }
        // drawWall(10,10,&rgb,width);
        rgb_to_yuv420p(rgb.data(), width, height, frame);
        frame->pts = i; 

        // Simple color pattern - cycles through brightness
        // int brightness = (i * 255) / (fps * duration_sec);
        // memset(frame->data[0], brightness, width * height);      // Y
        // memset(frame->data[1], puv[1], width * height / 4);         // U
        // memset(frame->data[2], puv[2], width * height / 4);         // V

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

void rgb_to_yuv420p(uint8_t* rgb_data, int width, int height, AVFrame* yuv_frame) {
    static SwsContext* sws_ctx = nullptr;

    if (!sws_ctx) {
        sws_ctx = sws_getContext(
            width, height, AV_PIX_FMT_RGB24,   // source
            width, height, AV_PIX_FMT_YUV420P, // destination
            SWS_BILINEAR,
            nullptr, nullptr, nullptr
        );
    }

    const uint8_t* src_slices[1] = { rgb_data };
    int src_stride[1] = { 3 * width };

    sws_scale(
        sws_ctx,
        src_slices,
        src_stride,
        0,
        height,
        yuv_frame->data,
        yuv_frame->linesize
    );
}

void drawWall(int x, int y, std::vector<uint8_t> *rgb, int width){
    const int tile_size = 40;
    const int wallR = 64;
    const int wallG = 64;
    const int wallB = 64;
    for (int i = y*tile_size; i < (y+1)*tile_size; i++) {
        for (int j = x*tile_size; j < (x+1)*tile_size; j++) {
            int k = (i * width + j) * 3;

            (*rgb)[k + 0] = wallR;      // R
            (*rgb)[k + 1] = wallG;      // G
            (*rgb)[k + 2] = wallB;      // B
        }
    }
}

void draw1_1(int x, int y, std::vector<uint8_t> *rgb, int width){
    drawFloor(x,y,rgb, width);
    const int tile_size = 40;
    const int wallR = 64;
    const int wallG = 64;
    const int wallB = 64;
    for (int i = y*tile_size; i < (y+1)*tile_size; i++) {
        for (int j = x*tile_size; j < (x+1)*tile_size-(40-i%tile_size); j++) {
            int k = (i * width + j) * 3;

            (*rgb)[k + 0] = wallR;      // R
            (*rgb)[k + 1] = wallG;      // G
            (*rgb)[k + 2] = wallB;      // B
        }
    }
}

void draw1_2(int x, int y, std::vector<uint8_t> *rgb, int width){
    const int tile_size = 40;
    const int wallR = 64;
    const int wallG = 64;
    const int wallB = 64;
    drawFloor(x,y,rgb, width);//TODO: Optimize this
    for (int i = y*tile_size; i < (y+1)*tile_size; i++) {
        for (int j = x*tile_size; j < (x+1)*tile_size-(i%tile_size); j++) {
            int k = (i * width + j) * 3;

            (*rgb)[k + 0] = wallR;      // R
            (*rgb)[k + 1] = wallG;      // G
            (*rgb)[k + 2] = wallB;      // B
        }
    }
}
void draw1_3(int x, int y, std::vector<uint8_t> *rgb, int width){
    const int tile_size = 40;
    const int wallR = 64;
    const int wallG = 64;
    const int wallB = 64;
    drawFloor(x,y,rgb, width);
    for (int i = y*tile_size; i < (y+1)*tile_size; i++) {
        for (int j = x*tile_size+(i%tile_size); j < (x+1)*tile_size; j++) {
            int k = (i * width + j) * 3;

            (*rgb)[k + 0] = wallR;      // R
            (*rgb)[k + 1] = wallG;      // G
            (*rgb)[k + 2] = wallB;      // B
        }
    }
}
void draw1_4(int x, int y, std::vector<uint8_t> *rgb, int width){
    const int tile_size = 40;
    const int wallR = 64;
    const int wallG = 64;
    const int wallB = 64;
    drawFloor(x,y,rgb, width);
    for (int i = y*tile_size; i < (y+1)*tile_size; i++) {
        for (int j = x*tile_size+(40-i%tile_size); j < (x+1)*tile_size; j++) {
            int k = (i * width + j) * 3;

            (*rgb)[k + 0] = wallR;      // R
            (*rgb)[k + 1] = wallG;      // G
            (*rgb)[k + 2] = wallB;      // B
        }
    }
}

void drawFloor(int x, int y, std::vector<uint8_t> *rgb, int width){
    const int tile_size = 40;
    const int floorR = 164;
    const int floorG = 164;
    const int floorB = 164;
    for (int i = y*tile_size; i < (y+1)*tile_size; i++) {
        for (int j = x*tile_size; j < (x+1)*tile_size; j++) {
            int k = (i * width + j) * 3;

            (*rgb)[k + 0] = floorR;      // R
            (*rgb)[k + 1] = floorG;      // G
            (*rgb)[k + 2] = floorB;      // B
        }
    }
}
