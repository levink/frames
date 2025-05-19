#pragma once 

#include "ffmpeg.h"

//todo: need this?
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

//todo: split .h + .cpp
static int64_t getFramePTS(const AVFrame* frame) {
    if (frame == nullptr) {
        return 0;
    }
        
    return frame->best_effort_timestamp == AV_NOPTS_VALUE ? 
        frame->pts : 
        frame->best_effort_timestamp;
}

struct FileReader {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* decoderContext = nullptr;
    SwsContext* swsContext = nullptr;
    const AVCodec* decoder = nullptr;
    int videoStreamIndex = -1;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    int64_t framePTS = 0;
    int64_t frameDuration = 0;
    uint8_t* pixelsRGB = nullptr;

private:
    //todo: need errorCode?
    ErrorCode openFile(const char* fileName) {

        if (avformat_open_input(&formatContext, fileName, nullptr, nullptr) < 0) {
            return ErrorCode::FileBadOpen;
        }

        if (avformat_find_stream_info(formatContext, nullptr) < 0) {
            return ErrorCode::StreamInfoNotFound;
        }

        videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
        if (videoStreamIndex < 0) {
            return ErrorCode::VideoStreamNotFound;
        }

        decoderContext = avcodec_alloc_context3(decoder);
        if (decoderContext == nullptr) {
            return ErrorCode::CodecContextBadAlloc;
        }

        const auto& videoStream = formatContext->streams[videoStreamIndex];
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
public:
    bool open(const char* fileName) {
        auto ret = openFile(fileName);
        //todo: save error info
        return ret == ErrorCode::Ok;
    }


private:

    bool readFrame() const {
        while (true) {
            int ret = av_read_frame(formatContext, packet);
            if (ret < 0) {
                return false;
            }

            if (packet->stream_index != videoStreamIndex) {
                av_packet_unref(packet);
                continue;
            }

            ret = avcodec_send_packet(decoderContext, packet);
            if (ret < 0) {
                return false;
            }

            ret = avcodec_receive_frame(decoderContext, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_packet_unref(packet);
                continue;
            }
            if (ret < 0) {
                av_packet_unref(packet);
                return false;
            }

            break;
        }
        return true;
    }
public:

    bool nextFrame() {
        if (readFrame()) {
            processFrame(frame);
            av_frame_unref(frame);
            av_packet_unref(packet);
            return true;
        }
        
        std::cout << "Finished" << std::endl;
        return false;
    }
    
    bool prevFrame() {

        std::cout << "seek to pts = " << framePTS - 1 << std::endl;
        int seek_res = av_seek_frame(formatContext, videoStreamIndex, framePTS - 1, AVSEEK_FLAG_BACKWARD);
        if (seek_res < 0) {
            return false;
        }
        avcodec_flush_buffers(decoderContext);

        bool frameFound = false;
        while (readFrame()) {
            int64_t pts = getFramePTS(frame);
            int64_t dur = frame->duration;
            if (pts + dur < framePTS) {
                av_frame_unref(frame);
                av_packet_unref(packet);
                continue;
            } 

            frameFound = true;
            break;
        }

        if (frameFound) {
            processFrame(frame);
            av_frame_unref(frame);
            av_packet_unref(packet);
        }
        return frameFound;
    }

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
    void processFrame(const AVFrame* frame) {
        static uint8_t* destFrame[AV_NUM_DATA_POINTERS] = { nullptr };
        static int destLineSize[AV_NUM_DATA_POINTERS] = { 0 };

        // Get RGB
        destFrame[0] = pixelsRGB;
        destLineSize[0] = 3 * frame->width;
        int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, destFrame, destLineSize);
        if (ret < 0) {
            std::cout << "processFrame: sws_scale() return " << ret << std::endl;
        }

        // Get frame info
        framePTS = getFramePTS(frame);
        frameDuration = frame->duration;

        int frameIndex = framePTS / 20; //for debug
        std::cout << "pts[" << frameIndex << "] = " << framePTS << " " << av_get_picture_type_char(frame->pict_type) << std::endl;

        /* auto retPNG = lodepng::encode("./testRGB.png", pixelsRGB, frame->width, frame->height, LodePNGColorType::LCT_RGB);
         if (retPNG) {
             std::cout << "lodepng::encode() on rgb: Error code=" << retPNG << std::endl;
         }*/
    }
};


