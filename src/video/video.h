#pragma once 
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
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
    int64_t durationPts;
    int64_t framesCount;
    int frameWidth;
    int frameHeight;
    float calcProgress(int64_t pts) const;
    int64_t toMicros(int64_t pts) const;
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
    FramePool& pool;
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
    State copyState();
    RGBFrame* readFrame(int8_t dir, int64_t seekPts);
    void saveResult(RGBFrame* frame);

public:
    FrameLoader(FramePool& pool, VideoReader& reader);
    ~FrameLoader();
    void start();
    void stop();
    void set(int8_t dir, int64_t pts);
    RGBFrame* getFrame();
    void putFrame(RGBFrame* unusedFrame);
};

struct FrameQueue {
    const size_t capacity = 5;
    const size_t deltaMin = 1;

    std::deque<RGBFrame*> items;
    size_t selected = -1;
    int8_t loadDir = 1;

    const RGBFrame* next();
    const RGBFrame* prev();
    void print() const;
    void play(FrameLoader& loader);
    void seekNextFrame(FrameLoader& loader);
    void seekPrevFrame(FrameLoader& loader);
    void fillFrom(FrameLoader& loader);

private:
    bool tooFarFromBegin() const;
    bool tooFarFromEnd() const;
    void tryFillBack(FrameLoader& loader);
    void tryFillFront(FrameLoader& loader);
    bool pushBack(RGBFrame* frame);
    bool pushFront(RGBFrame* frame);
    RGBFrame* popBack();
    RGBFrame* popFront();
    int64_t lastSeekPosition();
    int64_t firstSeekPosition();
};
