#include <iostream>
#include <algorithm>
#include "video.h"


static int64_t getFramePTS(const AVFrame* frame) {
    if (frame == nullptr) {
        return 0;
    }

    if (frame->best_effort_timestamp == AV_NOPTS_VALUE) {
        return frame->pts;
    }

    return frame->best_effort_timestamp;
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
void FramePool::put(RGBFrame* frame) {
    if (frame) {
        auto lock = std::lock_guard(mtx);
        frame->pts = -1;
        frame->duration = 0;
        items.push_back(frame);
    }
}
void FramePool::put(const std::vector<RGBFrame*>& frames) {
    auto lock = std::lock_guard(mtx);
    for (auto frame : frames) {
        frame->pts = -1;
        frame->duration = 0;
    }
    items.insert(items.end(), frames.begin(), frames.end());
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

bool FrameConverter::createContext(const AVCodecContext* decoder) {
    const auto& width = decoder->width;
    const auto& height = decoder->height;
    const auto& pixFmt = decoder->pix_fmt;

    swsContext = sws_getContext(
        width, height, pixFmt,
        width, height, AV_PIX_FMT_RGB24,
        SwsFlags::SWS_BILINEAR, nullptr, nullptr, nullptr);
    
    return swsContext;
}
void FrameConverter::destroyContext() {
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
}
int FrameConverter::toRGB(const AVFrame* frame, RGBFrame& result) {
    destFrame[0] = result.pixels;
    destLineSize[0] = 3 * frame->width;
    int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, destFrame, destLineSize);
    destFrame[0] = nullptr;
    destLineSize[0] = 0;
    return ret;
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
bool VideoReader::open(const char* fileName) {

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
bool VideoReader::read(RGBFrame& result) {
    if (readRaw()) {
        bool ok = convert(frame, result);
        av_frame_unref(frame);
        av_packet_unref(packet);
        return ok;
    }

    std::cout << "readFrame() finished" << std::endl;
    return false;
}
bool VideoReader::read(RGBFrame& result, int64_t skipPts) {
    while (readRaw()) {    
        auto pts = frame->pts;
        if (pts < skipPts) {
            av_frame_unref(frame);
            av_packet_unref(packet);
            continue;
        }

        bool ok = convert(frame, result);
        av_frame_unref(frame);
        av_packet_unref(packet);
        return ok;
    }

    std::cout << "readFrame() finished" << std::endl;
    return false;
}
bool VideoReader::readRaw() const {
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
bool VideoReader::convert(const AVFrame* frame, RGBFrame& result) {
    
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
bool VideoReader::seek(int64_t pts) {
    int result = av_seek_frame(formatContext, videoStreamIndex, pts, AVSEEK_FLAG_BACKWARD);
    if (result < 0) {
        return false;
    }
    avcodec_flush_buffers(decoderContext);
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


PlayLoop::PlayLoop(FramePool& pool, VideoReader& reader) :
    framePool(pool),
    reader(reader) { }
PlayLoop::~PlayLoop() {
    if (t.joinable()) {
        t.join();
    }
}
void PlayLoop::start() {
    finished.store(false);
    t = std::thread([this]() {
        playback();
    });
}
void PlayLoop::stop() {
    finished.store(true);
    cv.notify_one();
    if (t.joinable()) {
        t.join();
    }
}
void PlayLoop::set(int8_t dir, int64_t pts) {
    auto lock = std::lock_guard(mtx);

    //set state
    sharedState.dir = dir;
    sharedState.seekPts = pts;

    //flush
    auto tmp = result;
    result = nullptr;
    framePool.put(tmp);

    //wake up background thread
    cv.notify_one();
}
RGBFrame* PlayLoop::next() {
    auto lock = std::lock_guard(mtx);
    if (result == nullptr) {
        return nullptr;
    }

    auto tmp = result;
    result = nullptr;
    cv.notify_one();

    return tmp;
}
void PlayLoop::playback() {

    while (canRead()) {
        State copy = copyState();
        auto frame = readFrame(copy.dir, copy.seekPts);
        if (frame) {
            saveResult(frame);
        }
    }
    std::cout << "stopped" << std::endl;
}
bool PlayLoop::canRead() {
    auto lock = std::unique_lock(mtx);
    while (!finished && result) {
        cv.wait(lock);
    }
    return !finished;
}
RGBFrame* PlayLoop::readFrame(int8_t dir, int64_t seekPts) {

    if (dir > 0) {
        if (seekPts >= 0) {

            bool ok = reader.seek(seekPts);
            if (!ok) {
                return nullptr;
            }

            auto frame = framePool.get();
            ok = reader.read(*frame, seekPts);
            if (!ok) {
                framePool.put(frame);
                return nullptr;
            }

            return frame;
        }
        else {
            auto frame = framePool.get();
            bool ok = reader.read(*frame);
            if (!ok) {
                framePool.put(frame);
                return nullptr;
            }
            return frame;
        }
    }

    if (dir < 0) {

        if (seekPts < 0 && prevCache.empty()) {
            seekPts = lastPts - 1;
        }

        if (seekPts >= 0) {
            reader.seek(seekPts);
            
            prevCache.resize(5, nullptr);
            for (auto& item : prevCache) {
                if (item == nullptr) {
                    item = framePool.get();
                }
                item->pts = -1;
                item->duration = 0;
            }
            
            size_t writeIndex = 0;
            bool found = false;
            auto frame = prevCache[writeIndex];
            while (reader.read(*frame)) {
                auto min = frame->pts;
                auto max = frame->pts + frame->duration;
                if (min <= seekPts && seekPts < max) {
                    found = true;
                    break;
                }
                if (seekPts < min) {
                    break;
                }
                writeIndex = (writeIndex + 1) % prevCache.size();
                frame = prevCache[writeIndex];
            }

            if (found) {
                /*
                    cache.clearUnused();
                    cache.sortByPTS();
                */
                for (size_t i = 0; i < prevCache.size(); i++) {
                    if (prevCache[i]->pts == -1) {
                        framePool.put(prevCache[i]);
                        prevCache[i] = nullptr;
                    }
                }
                prevCache.erase(std::remove_if(prevCache.begin(), prevCache.end(), [](RGBFrame* item) {
                    return item == nullptr;
                }), prevCache.end());

                std::sort(prevCache.begin(), prevCache.end(), [](RGBFrame* left, RGBFrame* right) {
                    return left->pts < right->pts;
                });
            }
            else {
                // Not found. Need clear cache
                framePool.put(prevCache);
                prevCache.clear();
            }
        }
        
        if (prevCache.empty()) {
            return nullptr;
        }

        auto frame = prevCache.back();
        prevCache.pop_back();
        return frame;
    }

    return nullptr;
}
void PlayLoop::saveResult(RGBFrame* frame) {
    auto lock = std::unique_lock(mtx);
    result = frame;
    lastPts = frame->pts;
}
PlayLoop::State PlayLoop::copyState() {
    auto lock = std::lock_guard(mtx);
    auto copy = sharedState;
    sharedState.seekPts = -1;
    return copy;
}
