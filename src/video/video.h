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
    SeekBad              = 12,
    
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
    bool decoderHasData = false;

    ErrorCode open(const char* fileName);
    ErrorCode readNext();
    ErrorCode readPrev();
    void debug();

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
            av_frame_unref(frame);
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

void FileReader::debug() {
    int ret = 0;
    while (1) {
        ret = av_read_frame(formatContext, packet);
        if (ret < 0) {
            break;
        }

        if (packet->stream_index != videoIndex) {
            //std::cout << "av_read_frame skip, not a video" << std::endl;
            av_packet_unref(packet);
            continue;
        }
        else {
            //std::cout << "av_read_frame ok, found video frame" << std::endl;
        }

        ret = avcodec_send_packet(decoderContext, packet);
        if (ret < 0) {
            break;
        }
        ret = avcodec_receive_frame(decoderContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_unref(packet);
            continue;
        }
        if (ret >= 0) {
            processFrame(frame);
            av_frame_unref(frame);
        }
        av_packet_unref(packet);
    }
}

ErrorCode FileReader::readNext() {
    
    int ret = 0;
    while (1) {
        ret = av_read_frame(formatContext, packet);
        if (ret < 0) {
            break;
        }

        if (packet->stream_index != videoIndex) {
            av_packet_unref(packet);
            continue;
        }

        ret = avcodec_send_packet(decoderContext, packet);
        if (ret < 0) {
            break;
        }
        ret = avcodec_receive_frame(decoderContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_unref(packet);
            continue;
        }
        if (ret >= 0) {
            processFrame(frame);
            av_frame_unref(frame);
            av_packet_unref(packet);
            return ErrorCode::Ok;
        }
        av_packet_unref(packet);
    }

    std::cout << "Finished" << std::endl;
    return ErrorCode::Ok;
}

void FileReader::processFrame(const AVFrame* frame) const {
    static uint8_t* destFrame[AV_NUM_DATA_POINTERS] = { nullptr };
    static int destLineSize[AV_NUM_DATA_POINTERS] = { 0 };
    
    destFrame[0] = pixelsRGB;
    destLineSize[0] = 3 * frame->width;
    int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, destFrame, destLineSize);
    if (ret < 0) {
        std::cout << "processFrame: sws_scale() return " << ret << std::endl;
    }

    int64_t pts = frame->best_effort_timestamp == AV_NOPTS_VALUE ? frame->pts : frame->best_effort_timestamp;
    int frameIndex = pts / 20;
    std::cout << "pts[" << frameIndex << "] = " << pts << " " << av_get_picture_type_char(frame->pict_type) << std::endl;

   /* auto retPNG = lodepng::encode("./testRGB.png", pixelsRGB, frame->width, frame->height, LodePNGColorType::LCT_RGB);
    if (retPNG) {
        std::cout << "lodepng::encode() on rgb: Error code=" << retPNG << std::endl;
    }*/
}


ErrorCode FileReader::readPrev() {

    const AVStream* stream = formatContext->streams[videoIndex];
    const AVRational& time_base = stream->time_base;
    const AVRational& frame_rate = stream->r_frame_rate;
    int64_t num = static_cast<int64_t>(frame_rate.den) * time_base.den;
    int64_t den = static_cast<int64_t>(frame_rate.num) * time_base.num;
    int64_t duration = num / den;
    int64_t cur_pts = frame->best_effort_timestamp == AV_NOPTS_VALUE ? frame->pts : frame->best_effort_timestamp;
    int64_t prev_pts = cur_pts > 2 * duration ? (cur_pts - 2 * duration) : 0;
    

    int64_t pts = frame->best_effort_timestamp == AV_NOPTS_VALUE ? frame->pts : frame->best_effort_timestamp;
    int64_t frameIndex = pts / 20 - 1;

    int seek_res = av_seek_frame(formatContext, videoIndex, pts - 20, AVSEEK_FLAG_BACKWARD);
    if (seek_res < 0) {
        return ErrorCode::SeekBad;
    }
    avcodec_flush_buffers(decoderContext);
    return readNext();
}