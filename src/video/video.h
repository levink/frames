#pragma once 
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "ffmpeg.h"
#include "frame.h"

using std::mutex;
using std::vector;

class FramePool {
    mutex mtx;
    vector<RGBFrame*> items;
    int frameWidth = 0;
    int frameHeight = 0;

public:
    FramePool() = default;
    ~FramePool();
    void createFrames(size_t count, int w, int h);
    void put(RGBFrame* item);
    void put(const vector<RGBFrame*>& frames);
    RGBFrame* get();
};

struct StreamInfo {
    AVRational time_base;
    int64_t duration;
    int64_t nb_frames;
    float calcProgress(int64_t pts) const {
        if (duration == 0) {
            return 0;
        }

        float value = (pts * 100.f) / duration;
        if (value < 0.f) return 0;
        if (value > 100.f) return 100.f;
        return value;
    }
};

struct FrameConverter {
    SwsContext* swsContext = nullptr;
    uint8_t* destFrame[AV_NUM_DATA_POINTERS] = { nullptr };
    int destLineSize[AV_NUM_DATA_POINTERS] = { 0 };

    bool createContext(const AVCodecContext* decoder);
    void destroyContext();
    int toRGB(const AVFrame* frame, RGBFrame& result);
};

struct VideoReader {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* decoderContext = nullptr;
    const AVCodec* decoder = nullptr;
    int videoStreamIndex = -1;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    FrameConverter converter;

    VideoReader();
    ~VideoReader();

    bool open(const char* fileName);
    bool read(RGBFrame& result);
    bool read(RGBFrame& result, int64_t skipPts);
    bool seek(int64_t pts);

    StreamInfo getStreamInfo() const;

private:
    bool readRaw() const;
    bool convert(const AVFrame* frame, RGBFrame& result);
};
           
class FrameLoader {
    std::thread t;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> finished = false;
    FramePool& framePool;
    VideoReader& reader;

    struct State {
        int8_t dir = 1;
        int64_t seekPts = -1;
    } sharedState;

    RGBFrame* result = nullptr;
    vector<RGBFrame*> prevCache;
    int64_t lastPts = -1;

    void playback();
    bool canRead();
    RGBFrame* readFrame(int8_t dir, int64_t seekPts);
    void saveResult(RGBFrame* frame);
    State copyState();

public:
    FrameLoader(FramePool& pool, VideoReader& reader);
    ~FrameLoader();
    void start();
    void stop();
    void set(int8_t dir, int64_t pts);
    RGBFrame* next();
    void putUnused(RGBFrame* frame);
};

