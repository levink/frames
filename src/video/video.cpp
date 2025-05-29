#include <iostream>
#include "video.h"

bool RGBConverter::createContext(const AVCodecContext* decoder) {
    const auto& width = decoder->width;
    const auto& height = decoder->height;
    const auto& pixFmt = decoder->pix_fmt;

    swsContext = sws_getContext(
        width, height, pixFmt,
        width, height, AV_PIX_FMT_RGB24,
        SwsFlags::SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    return swsContext;
}
void RGBConverter::destroyContext() {
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
}
int RGBConverter::toRGB(const AVFrame* frame, RGBFrame& result) {
    destFrame[0] = result.pixels;
    destLineSize[0] = 3 * frame->width;
    int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, destFrame, destLineSize);
    destFrame[0] = nullptr;
    destLineSize[0] = 0;
    return ret;
}


static int64_t getFramePTS(const AVFrame* frame) {
    if (frame == nullptr) {
        return 0;
    }

    if (frame->best_effort_timestamp == AV_NOPTS_VALUE) {
        return frame->pts;
    }

    return frame->best_effort_timestamp;
}
VideoReader::VideoReader() {
    av_log_set_level(AV_LOG_FATAL);
}
VideoReader::~VideoReader() {
    if (formatContext) {
        avformat_close_input(&formatContext);
    }
    if (decoderContext) {
        avcodec_free_context(&decoderContext);
    }
    if (packet) {
        av_packet_free(&packet);
    }
    if (frame) {
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
    converter.destroyContext();
}
bool VideoReader::openFile(const char* fileName) {

    if (avformat_open_input(&formatContext, fileName, nullptr, nullptr) < 0) {
        return false;// OpenFileResult::FileBadOpen;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        return false;// OpenFileResult::StreamInfoNotFound;
    }

    videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (videoStreamIndex < 0) {
        return false;// OpenFileResult::VideoStreamNotFound;
    }

    decoderContext = avcodec_alloc_context3(decoder);
    if (decoderContext == nullptr) {
        return false;// OpenFileResult::CodecContextBadAlloc;
    }

    const AVStream* videoStream = formatContext->streams[videoStreamIndex];
    if (avcodec_parameters_to_context(decoderContext, videoStream->codecpar) < 0) {
        return false;// OpenFileResult::CodecContextBadCopy;
    }

    if (avcodec_open2(decoderContext, decoder, nullptr) < 0) {
        return false;// OpenFileResult::CodecContextBadInit;
    }

    packet = av_packet_alloc();
    if (packet == nullptr) {
        return false;// OpenFileResult::PacketBadAlloc;
    }

    frame = av_frame_alloc();
    if (frame == nullptr) {
        return false;// OpenFileResult::FrameBadAlloc;
    }

    if (converter.createContext(decoderContext) == false) {
        return false;// OpenFileResult::SwsContextBadAlloc;
    }

    return true;// OpenFileResult::Ok;
}
bool VideoReader::nextFrame(RGBFrame& result) {
    if (readFrame()) {
        bool ok = toRGB(frame, result);
        av_frame_unref(frame);
        av_packet_unref(packet);
        return ok;
    }

    std::cout << "readFrame() finished" << std::endl;
    return false;
}
bool VideoReader::prevFrame(int64_t pts, RGBFrame& result) {
    //todo: case for (pts < 0) ?

    int seek_res = av_seek_frame(formatContext, videoStreamIndex, pts, AVSEEK_FLAG_BACKWARD);
    if (seek_res < 0) {
        return false;
    }
    avcodec_flush_buffers(decoderContext);

    bool frameFound = false;
    while (readFrame()) {
        int64_t framePts = getFramePTS(frame);
        int64_t frameDur = frame->duration;
        if (framePts + frameDur < pts) {
            av_frame_unref(frame);
            av_packet_unref(packet);
            continue;
        }

        frameFound = true;
        break;
    }

    if (frameFound) {
        bool ok = toRGB(frame, result);
        av_frame_unref(frame);
        av_packet_unref(packet);
        return ok;
    }

    return false;
}
bool VideoReader::readFrame() const {
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
bool VideoReader::toRGB(const AVFrame* frame, RGBFrame& result) {
    
    bool sameSize = result.checkSize(frame->width, frame->height);
    if (!sameSize) {
        std::cout << "toRGB(). Bad image size" << std::endl;
        return false;
    }

    int ret = converter.toRGB(frame, result);
    if (ret < 0) {
        std::cout << "toRGB(). Bad convert, ret = " << ret << std::endl;
        return false;
    }

    result.pts = getFramePTS(frame);
    result.duration = frame->duration;
    return true;
}
StreamInfo VideoReader::getStreamInfo() const {
    if (videoStreamIndex < 0 || formatContext->nb_streams <= videoStreamIndex) {
        return StreamInfo{ {0, 1}, 0, 0 };
    }
    const AVStream* stream = formatContext->streams[videoStreamIndex];
    return StreamInfo {
        stream->time_base,
        stream->duration,
        stream->nb_frames
    };
}


FramePool::~FramePool() {
    for (auto& item : items) {
        delete item;
    }
}
void FramePool::createFrames(size_t count, int w, int h) {
    auto lock = std::lock_guard(mtx);

    frameWidth = w;
    frameHeight = h;
    items.reserve(items.size() + count);
    for (size_t i = 0; i < count; i++) {
        items.emplace_back(new RGBFrame(w, h));
    }
}
void FramePool::put(RGBFrame* item) {
    auto lock = std::lock_guard(mtx);
    items.push_back(item);
}
RGBFrame* FramePool::get() {
    auto lock = std::lock_guard(mtx);

    if (items.empty()) {
        return new RGBFrame(frameWidth, frameHeight);
    }

    auto last = items.back();
    items.pop_back();

    return last;
}


FrameChannel::~FrameChannel() {
    delete cached;
}
bool FrameChannel::put(RGBFrame* newFrame) {
    if (closed) {
        return false;
    }

    auto lock = std::unique_lock(mtx);
    while (!closed && cached) {
        cv.wait(lock);
    }

    if (closed) {
        return false;
    }

    cached = newFrame;
    return true;
}
RGBFrame* FrameChannel::get() {
    auto lock = std::lock_guard(mtx);
    if (cached == nullptr) {
        return nullptr;
    }

    RGBFrame* result = cached;
    cached = nullptr;
    cv.notify_one();
    return result;
}
void FrameChannel::close() {
    auto lock = std::unique_lock(mtx);
    closed.store(true);
    cv.notify_one();
}


PlayLoop::PlayLoop(FramePool& pool, FrameChannel& channel, VideoReader& reader) :
    framePool(pool),
    toChannel(channel),
    reader(reader) {
}
PlayLoop::~PlayLoop() {
    if (t.joinable()) {
        t.join();
    }
}
void PlayLoop::startThread() {
    finished.store(false);

    t = std::thread([this]() {

        while (!finished) {

            RGBFrame* frame = framePool.get();
            bool ok = reader.nextFrame(*frame);
            if (!ok) {
                framePool.put(frame);
                continue;
            }

            bool sent = toChannel.put(frame); //<-- cv wait here
            if (!sent) {
                framePool.put(frame);
                return;
            }
        }
        std::cout << "stopped" << std::endl;
    });
}
void PlayLoop::stopThread() {
    finished.store(true);
    toChannel.close();
    if (t.joinable()) {
        t.join();
    }
}
