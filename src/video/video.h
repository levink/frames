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
    StreamNotFound = 2,
    VideoStreamNotFound = 3,
    CodecContextBadAlloc = 4,
    CodecContextBadCopy = 5,
    CodecContextBadInit = 6,
    SwsContextBadAlloc  = 7,
    
    PacketBadAlloc = 8,
    FrameBadAlloc  = 9,

    Unknown
};

struct FileInfo {

    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    const AVCodec* decoder = nullptr;
    int videoIndex = -1;

    SwsContext* swsContext = nullptr;
    uint8_t* pixelsRGBA = nullptr;
    int pixelsLineSize = 0;

    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;

    ErrorCode openFile(const char* fileName);
    void processFrame(AVFrame* frame);
    void nextFrame();

    FileInfo() {
        av_log_set_level(AV_LOG_FATAL);
    }

    ~FileInfo() {
        if (formatContext) {
            avformat_free_context(formatContext);
        }
        if (codecContext) {
            avcodec_free_context(&codecContext);
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

        delete[] pixelsRGBA;
        pixelsRGBA = nullptr;
        pixelsLineSize = 0;
    }
};

ErrorCode FileInfo::openFile(const char* fileName) {

    if (avformat_open_input(&formatContext, fileName, nullptr, nullptr) < 0) {
        return ErrorCode::FileBadOpen;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        return ErrorCode::StreamNotFound;
    }

    videoIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (videoIndex < 0) {
        return ErrorCode::VideoStreamNotFound;
    }

    codecContext = avcodec_alloc_context3(decoder);
    if (codecContext == nullptr) {
        return ErrorCode::CodecContextBadAlloc;
    }

    const auto& videoStream = formatContext->streams[videoIndex];
    if (avcodec_parameters_to_context(codecContext, videoStream->codecpar) < 0) {
        return ErrorCode::CodecContextBadCopy;
    }

    if (avcodec_open2(codecContext, decoder, nullptr) < 0) {
        return ErrorCode::CodecContextBadInit;
    }

    {
        //why here?
        auto width = codecContext->width;
        auto height = codecContext->height;
        swsContext = sws_getContext(
            width, height, codecContext->pix_fmt,
            width, height, AV_PIX_FMT_RGB0,
            SwsFlags::SWS_BILINEAR,  // or SWS_FAST_BILINEAR?
            nullptr, nullptr, nullptr);
        if (!swsContext) {
            return ErrorCode::SwsContextBadAlloc;
        }

        pixelsRGBA = new uint8_t[width * height * 4];
        pixelsLineSize = width * 4;
    }


    packet = av_packet_alloc();
    if (packet == nullptr) {
        return ErrorCode::PacketBadAlloc;
    }

    frame = av_frame_alloc();
    if (!frame) {
        return ErrorCode::FrameBadAlloc;
    }

    int frameSkip = 100;
    bool finished = false;
    auto errCode = ErrorCode::Ok;
    while (av_read_frame(formatContext, packet) >= 0 && !finished) {

        PacketHolder packetHolder{ packet };
        int response = avcodec_send_packet(codecContext, packetHolder.data);
        if (packet->stream_index != videoIndex) {
            continue;
        }

        while (response >= 0) {
            response = avcodec_receive_frame(codecContext, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            }
            else if (response < 0) {
                errCode = ErrorCode::Unknown; 
                //error = "Error on receive frame: " + std::to_string(response);
                finished = true;
                break;
            }

            if (frameSkip > 0) {
                frameSkip--;
                continue;
            }

            processFrame(frame);
            finished = true;
            break;
        }
    }

    return errCode;
}

void FileInfo::processFrame(AVFrame* frame) {

    const auto width = frame->width;
    const auto height = frame->height;
    const auto lineSize = frame->linesize[0];

    int destLineSize[4] = { pixelsLineSize, 0, 0, 0 };
    uint8_t* dest[4] = { pixelsRGBA, nullptr, nullptr, nullptr };
    int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, dest, destLineSize);
    if (ret < 0) {
        std::cout << "processFrame: sws_scale() return " << ret << std::endl;
    }

  /*  auto retPNG = lodepng::encode("./testRGB.png", pixelsRGBA, width, height, LodePNGColorType::LCT_RGBA);
    if (retPNG) {
        std::cout << "lodepng::encode() on rgb: Error code=" << retPNG << std::endl;
    }*/
}

void FileInfo::nextFrame() {

}