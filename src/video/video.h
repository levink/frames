#pragma once 

#include "ffmpeg.h"

enum class ErrorCode {
    Ok                   = 0,
    FileBadOpen          = 1,
    StreamInfoNotFound   = 2,
    VideoStreamNotFound  = 3,
    CodecContextBadAlloc = 4,
    CodecContextBadCopy  = 5,
    CodecContextBadInit  = 6,
    SwsContextBadAlloc   = 7,
    PacketBadAlloc       = 8,
    FrameBadAlloc        = 9,
    ReadBadSent          = 10,
    ReadBadReceive       = 11,
    
    Unknown
};

struct FileReader {

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* decoderContext = nullptr;
    SwsContext* swsContext = nullptr;
    const AVCodec* decoder = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    uint8_t* pixelsRGB = nullptr;
    int videoIndex = -1;
    bool hasWork = false;

    ErrorCode open(const char* fileName);
    ErrorCode read();

    FileReader() {
        av_log_set_level(AV_LOG_FATAL);
    }

    ~FileReader() {
        if (formatContext) {
            avformat_close_input(&formatContext);
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

private:
    void processFrame(const AVFrame* frame) const;
};

ErrorCode FileReader::open(const char* fileName) {

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

ErrorCode FileReader::read() {

    // continue work from previous call
    if (hasWork) {
        int recv = avcodec_receive_frame(decoderContext, frame);
        if (recv == 0 ) {
            processFrame(frame);
            return ErrorCode::Ok;
        }

        hasWork = false;
        av_packet_unref(packet);

        if (recv != AVERROR(EAGAIN)) {
            return ErrorCode::ReadBadReceive;
        }
    }
    
    while (av_read_frame(formatContext, packet) >= 0) {
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
        if (recv == 0) {
            processFrame(frame);
            hasWork = true;
            return ErrorCode::Ok;
        }
        av_packet_unref(packet);

        if (recv != AVERROR(EAGAIN)) {
            return ErrorCode::ReadBadReceive;
        } 
    }

    return ErrorCode::Ok;
}

void FileReader::processFrame(const AVFrame* frame) const {
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
