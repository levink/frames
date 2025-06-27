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
static int64_t prevSeekPosition(const RGBFrame* from) {
    return from->pts - 1;
}
static int64_t nextSeekPosition(const RGBFrame* from) {
    return from->pts + from->duration;
}
static bool checkPts(const RGBFrame* left, const RGBFrame* right) {
    return left->pts + left->duration == right->pts;
}


FramePool::~FramePool() {
    for (auto& item : items) {
        delete item;
    }
}
void FramePool::createFrames(size_t count, int w, int h) {
    auto lock = std::lock_guard(mtx);

    for (auto& item : items) {
        delete item;
        item = nullptr;
    }
    items.clear();
    items.resize(count);
    for (size_t i = 0; i < count; i++) {
        items[i] = new RGBFrame(w, h);
    }

    frameWidth = w;
    frameHeight = h;

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


float StreamInfo::calcProgress(int64_t pts) const {
    if (durationPts == 0) {
        return 0;
    }

    float value = (pts * 100.f) / durationPts;
    if (value < 0.f) return 0;
    if (value > 100.f) return 100.f;
    return value;
}
int64_t StreamInfo::ptsToMicros(int64_t pts) const {
    auto num = pts * time_base.num * 1000000;
    auto den = time_base.den;
    return num / den;
}
int64_t StreamInfo::microsToPts(int64_t micros) const {
    auto num = micros * time_base.den;
    auto den = 1000000 * time_base.num;
    return num / den;
}
int64_t StreamInfo::progressToPts(float progress) const {
    return (progress * durationPts) / 100.f; //todo: trunc [0, maxPts]?
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
    destLineSize[0] = RGBFrame::BYTES_PER_PIXEL * frame->width;
    int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, destFrame, destLineSize);
    destFrame[0] = nullptr;
    destLineSize[0] = 0;
    return ret;
}


VideoReader::VideoReader() {
    av_log_set_level(AV_LOG_FATAL);
}
VideoReader::~VideoReader() {
    destroy();
}
void VideoReader::destroy() {
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
    if (decoderContext) {
        avcodec_free_context(&decoderContext);
        decoderContext = nullptr;
    }
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (frame) {
        av_frame_unref(frame);
        av_frame_free(&frame);
    }
    converter.destroyContext();
}
bool VideoReader::open(const char* fileName) {
    destroy();
    eof = false;

    if (avformat_open_input(&formatContext, fileName, nullptr, nullptr) < 0) {
        return false;// OpenFileResult::FileBadOpen;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        return false;// OpenFileResult::StreamInfoNotFound;
    }

    const AVCodec* decoder = nullptr;
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
bool VideoReader::read(RGBFrame& result, int64_t skipPts) {
    while (readRaw()) {    
        if (frame->pts < skipPts) {
            av_frame_unref(frame);
            av_packet_unref(packet);
            continue;
        }

        bool ok = convert(frame, result);
        av_frame_unref(frame);
        av_packet_unref(packet);
        return ok;
    }

    std::cout << "readFrame() finished2" << std::endl;
    return false;
}
bool VideoReader::readRaw() {
    while (true) {
        int ret = av_read_frame(formatContext, packet);
        if (ret == AVERROR_EOF) {
            eof = true;
            return false;
        }
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
    eof = false;
    return true;
}
StreamInfo VideoReader::getStreamInfo() const {
    if (videoStreamIndex < 0 || formatContext->nb_streams <= videoStreamIndex) {
        return StreamInfo{ {0, 1}, 0, 0, 1, 1 };
    }

    const AVStream* stream = formatContext->streams[videoStreamIndex];
    return StreamInfo {
        stream->time_base,
        stream->duration,
        stream->nb_frames,
        decoderContext->width,
        decoderContext->height
    };
}

FrameLoader::~FrameLoader() {
    if (t.joinable()) {
        t.join();
    }

    for (auto& item : prevCache) {
        delete item;
        item = nullptr;
    }
    
    delete result;
    result = nullptr;
}
bool FrameLoader::open(const char* fileName, StreamInfo& info) {
    if (reader.open(fileName)) {
        info = reader.getStreamInfo();
        return true;
    }
    return false;
}
void FrameLoader::start() {
    finished.store(false);
    {
        auto lock = std::lock_guard(mtx);
        sharedState.loadDir = 1;
        sharedState.seekPts = -1;
    }
    cv.notify_one();
    t = std::thread([this]() {
        playback();
    });
}
void FrameLoader::stop() {
    if (finished) {
        return;
    }

    finished.store(true);
    { auto lock = std::lock_guard(mtx); }
    cv.notify_one();
    if (t.joinable()) {
        t.join();
    }

    pool.put(prevCache);
    prevCache.clear();

    pool.put(result);
    result = nullptr;
}
void FrameLoader::seek(int8_t loadDir, int64_t seekPts) {
    auto lock = std::lock_guard(mtx);

    //set state
    sharedState.loadDir = loadDir;
    sharedState.seekPts = seekPts;

    //flush
    auto frame = result;
    result = nullptr;
    pool.put(frame);

    //wake up background thread
    cv.notify_one();
}
void FrameLoader::playback() {
    while (canWork()) {
        auto state = copyState();
        auto frame = readFrame(state);
        saveResult(frame, state);
    }
    std::cout << "stopped" << std::endl;
}
bool FrameLoader::canWork() {
    auto lock = std::unique_lock(mtx);
    while (!finished && (result || reader.eof && sharedState.seekPts == -1)) {
        cv.wait(lock);
    }
    return !finished;
}
FrameLoader::State FrameLoader::copyState() {
    auto lock = std::lock_guard(mtx);
    auto copy = sharedState;
    return copy;
}
void FrameLoader::saveResult(RGBFrame* frame, State workState) {
    auto lock = std::lock_guard(mtx);
    if (workState != sharedState) {
        /* result is not actual because sharedState changed during the av_read... */
        pool.put(frame);
        return;
    }

    if (sharedState.seekPts != -1) {
        // need seek only once per read
        sharedState.seekPts = -1;
    }

    if (frame) {
        result = frame;
        lastPts = frame->pts;
    }
}
RGBFrame* FrameLoader::readFrame(const State& state) {

    auto loadDir = state.loadDir;
    auto seekPts = state.seekPts;

    if (loadDir > 0) {
        if (seekPts >= 0) {

            bool ok = reader.seek(seekPts);
            if (!ok) {
                return nullptr;
            }

            auto frame = pool.get();
            ok = reader.read(*frame, seekPts);
            if (!ok) {
                pool.put(frame);
                return nullptr;
            }

            return frame;
        }
        else {
            auto frame = pool.get();
            bool ok = reader.read(*frame);
            if (!ok) {
                pool.put(frame);
                return nullptr;
            }
            return frame;
        }
    }

    if (loadDir < 0) {

        if (seekPts < 0 && prevCache.empty()) {
            seekPts = lastPts - 1;
        }

        if (seekPts >= 0) {
            reader.seek(seekPts);
            
            prevCache.resize(5, nullptr);
            for (auto& item : prevCache) {
                if (item == nullptr) {
                    item = pool.get();
                }
                item->pts = -1;
                item->duration = 0;
            }
            
            size_t writeIndex = 0;
            bool foundInCache = false;
            auto frame = prevCache[writeIndex];
            while (reader.read(*frame)) {
                auto min = frame->pts;
                auto max = frame->pts + frame->duration;
                if (min <= seekPts && seekPts < max) {
                    foundInCache = true;
                    break;
                }
                if (seekPts < min) {
                    break;
                }
                writeIndex = (writeIndex + 1) % prevCache.size();
                frame = prevCache[writeIndex];
            }

            if (foundInCache) {
                // cache.clearUnused();
                for (size_t i = 0; i < prevCache.size(); i++) {
                    if (prevCache[i]->pts == -1) {
                        pool.put(prevCache[i]);
                        prevCache[i] = nullptr;
                    }
                }
                prevCache.erase(std::remove_if(prevCache.begin(), prevCache.end(), [](RGBFrame* item) {
                    return item == nullptr;
                }), prevCache.end());

                // cache.sortByPTS();
                std::sort(prevCache.begin(), prevCache.end(), [](RGBFrame* left, RGBFrame* right) {
                    return left->pts < right->pts;
                });
            }
            else {
                // Not found. Need clear cache
                pool.put(prevCache);
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
RGBFrame* FrameLoader::getFrame() {
    auto lock = std::lock_guard(mtx);
    if (result == nullptr) {
        return nullptr;
    }

    auto tmp = result;
    result = nullptr;
    cv.notify_one();

    return tmp;
}
void FrameLoader::putFrame(RGBFrame* unusedFrame) {
    pool.put(unusedFrame);
}
void FrameLoader::createFrames(size_t count, int w, int h) {
    pool.createFrames(count, w, h);
}


const RGBFrame* FrameQueue::curr() {
    if (0 <= selected && selected < items.size()) {
        return items[selected];
    }
    return nullptr;
}
const RGBFrame* FrameQueue::next() {
    auto nextIndex = selected + 1;
    if (0 <= nextIndex && nextIndex < items.size()) {
        selected = nextIndex;
        return items[selected];
    }
    return nullptr;
}
void FrameQueue::print() const {
    std::cout << "[";
    for (size_t i = 0; i < items.size(); i++) {
        auto elem = items[i];
        char mark = (i == selected) ? '=' : ' ';
        std::cout << elem->pts << mark << " ";
    }
    std::cout << "] ";

    if (0 <= selected && selected < items.size()) {
        std::cout << "pts=" << items[selected]->pts;
    } else {
        std::cout << "pts=???";
    }
    std::cout << std::endl;
}
void FrameQueue::play(FrameLoader& loader) {
    if (loadDir < 0) {
        loadDir = 1;
        auto seekPos = items.empty() ? 
            0 : //todo: maybe playState.framePts is better?
            nextSeekPosition(items.back()); 
        loader.seek(loadDir, seekPos);
    }
}
void FrameQueue::seekNextFrame(FrameLoader& loader) {
    if (selected + 1 < items.size()) {
        selected++;
    }

    if (loadDir < 0 && !tooFarFromEnd() && !items.empty()) {
        loadDir = 1;
        auto seekPos = nextSeekPosition(items.back());
        loader.seek(loadDir, seekPos);
    }
}
void FrameQueue::seekPrevFrame(FrameLoader& loader) {
    if (selected > 0) {
        selected--;
    }

    if (loadDir > 0 && !tooFarFromBegin() && !items.empty()) {
        loadDir = -1;
        auto seekPos = prevSeekPosition(items.front());
        loader.seek(loadDir, seekPos);
    }
}
void FrameQueue::fillFrom(FrameLoader& loader) {
    if (loadDir > 0) {
        tryFillBack(loader);
        return;
    }
    if (loadDir < 0) {
        tryFillFront(loader);
        return;
    }
}
void FrameQueue::flush(FrameLoader& loader) {
    for (auto item : items) {
        loader.putFrame(item);
    }
    items.clear();
    selected = 0;
    loadDir = 1;
}
bool FrameQueue::tooFarFromBegin() const {
    return selected > deltaMin;
}
bool FrameQueue::tooFarFromEnd() const {
    return selected + deltaMin + 1 < items.size();
}
void FrameQueue::tryFillBack(FrameLoader& loader) {
    if (tooFarFromEnd()) {
        return;
    }

    auto frame = loader.getFrame();
    if (!frame) {
        return;
    }

    if (!items.empty() && !checkPts(items.back(), frame)) {
        std::cout << "Warning: skip frame, bad pts" << std::endl;
    }

    auto prev = items.pushBack(frame);
    if (prev) {
        loader.putFrame(prev);
        if (selected > 0) {
            selected--;
        }
    }
}
void FrameQueue::tryFillFront(FrameLoader& loader) {
    if (tooFarFromBegin()) {
        return;
    }

    auto frame = loader.getFrame();
    if (!frame) {
        return;
    }

    if (!items.empty() && !checkPts(frame, items.front())) {
        std::cout << "Warning: skip frame, bad pts" << std::endl;
    }

    auto prev = items.pushFront(frame);
    loader.putFrame(prev);
    
    if (selected >= 0) {
        selected++;
    }
}
