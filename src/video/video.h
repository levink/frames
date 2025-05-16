#pragma once 

#include "ffmpeg.h"

struct PacketHolder {
    AVPacket* data = nullptr;
    ~PacketHolder() {
        if (data) {
            av_packet_unref(data);
        }
    }
};

enum class ErrorCode {
    Ok = 0,
    FileBadOpen = 1,
    StreamInfoNotFound = 2,
    VideoStreamNotFound = 3,
    CodecContextBadAlloc = 4,
    CodecContextBadCopy = 5,
    CodecContextBadInit = 6,
    SwsContextBadAlloc  = 7,
    PacketBadAlloc = 8,
    FrameBadAlloc  = 9,
    ReadBadSent    = 10,
    ReadBadReceive = 11,
    
    
    Unknown
};

struct FileInfo {

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* decoderContext = nullptr;
    SwsContext* swsContext = nullptr;
    const AVCodec* decoder = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    uint8_t* pixelsRGB = nullptr;
    int videoIndex = -1;

    ErrorCode open(const char* fileName);
    ErrorCode read();

    void processFrame(AVFrame* frame);
    void nextFrame();

    FileInfo() {
        av_log_set_level(AV_LOG_FATAL);
    }

    ~FileInfo() {
        if (formatContext) {
            avformat_free_context(formatContext);
        }
        if (decoderContext) {
            avcodec_free_context(&decoderContext);
        }
        if (swsContext) {
            sws_freeContext(swsContext);
        }
        if (packet) {
            av_packet_free(&packet);
        }
        if (frame) {
            av_frame_free(&frame);
        }

        delete[] pixelsRGB;
    }
};

ErrorCode FileInfo::open(const char* fileName) {

    if (avformat_open_input(&formatContext, fileName, nullptr, nullptr) < 0) {
        return ErrorCode::FileBadOpen;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        return ErrorCode::StreamInfoNotFound;
    }

    videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (videoIndex < 0) {
        return ErrorCode::VideoStreamNotFound;
    }

    decoderContext = avcodec_alloc_context3(decoder);
    if (decoderContext == nullptr) {
        return ErrorCode::CodecContextBadAlloc;
    }

    const auto& videoStream = formatContext->streams[videoIndex];
    if (avcodec_parameters_to_context(decoderContext, videoStream->codecpar) < 0) {
        return ErrorCode::CodecContextBadCopy;
    }

    if (avcodec_open2(decoderContext, decoder, nullptr) < 0) {
        return ErrorCode::CodecContextBadInit;
    }

    packet = av_packet_alloc();
    if (packet == nullptr) {
        return ErrorCode::PacketBadAlloc;
    }

    frame = av_frame_alloc();
    if (frame == nullptr) {
        return ErrorCode::FrameBadAlloc;
    }

    const auto& width = decoderContext->width;
    const auto& height = decoderContext->height;
    pixelsRGB = new uint8_t[width * height * 3];
    swsContext = sws_getContext(
        width, height, decoderContext->pix_fmt,
        width, height, AV_PIX_FMT_RGB24,
        SwsFlags::SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (swsContext == nullptr) {
        return ErrorCode::SwsContextBadAlloc;
    }

    return ErrorCode::Ok;
}

ErrorCode FileInfo::read() {

    bool processed = false;
    while (av_read_frame(formatContext, packet) >= 0 && !processed) {
        if (packet->stream_index != videoIndex) {
            continue;
        }

        // Send packet to decoder
        int send = avcodec_send_packet(decoderContext, packet);
        if (send != 0) {
            av_packet_unref(packet);
            return ErrorCode::ReadBadSent;
        }

        // Receive frame from decoder
        int recv = avcodec_receive_frame(decoderContext, frame);
        while (recv == 0) {
            processFrame(frame);
            processed = true;
            recv = avcodec_receive_frame(decoderContext, frame);
        }
        av_packet_unref(packet);

        if (recv != AVERROR(EAGAIN)) {
            return ErrorCode::ReadBadReceive;
        } 
    }

    return ErrorCode::Ok;
}


void FileInfo::processFrame(AVFrame* frame) {
    uint8_t* destFrame[4] = { pixelsRGB, nullptr, nullptr, nullptr };
    int destLineSize[4] = { frame->width * 3, 0, 0, 0 };
    
    int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, destFrame, destLineSize);
    if (ret < 0) {
        std::cout << "processFrame: sws_scale() return " << ret << std::endl;
    }

   /* auto retPNG = lodepng::encode("./testRGB.png", pixelsRGB, frame->width, frame->height, LodePNGColorType::LCT_RGB);
    if (retPNG) {
        std::cout << "lodepng::encode() on rgb: Error code=" << retPNG << std::endl;
    }*/
}

void FileInfo::nextFrame() {

}