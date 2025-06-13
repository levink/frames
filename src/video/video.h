#pragma once 
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "ffmpeg.h"
#include "frame.h"
#include "util/circlebuffer.h"

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
    int64_t PtsToMicros(int64_t pts) const;
    int64_t MicrosToPts(int64_t micros) const;
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
    bool eof = false;

    VideoReader();
    ~VideoReader();

    bool open(const char* fileName);
    bool read(RGBFrame& result);
    bool read(RGBFrame& result, int64_t skipPts);
    bool seek(int64_t pts);
    StreamInfo getStreamInfo() const;

private:
    bool readRaw();
    bool convert(const AVFrame* frame, RGBFrame& result);
};
           
class FrameLoader {
    std::thread t;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> finished = false;
    FramePool pool;
    VideoReader& reader;

    struct State {
        int8_t loadDir = 1;
        int64_t seekPts = -1;
        friend bool operator==(const State& left, const State& right) {
            return 
                left.loadDir == right.loadDir &&
                left.seekPts == right.seekPts;
        }
    } sharedState;

    RGBFrame* result = nullptr;
    vector<RGBFrame*> prevCache;
    int64_t lastPts = -1;

    void playback();
    bool canWork();
    State copyState();
    void saveResult(RGBFrame* frame, State state);
    RGBFrame* readFrame(const State& state);

public:
    FrameLoader(VideoReader& reader);
    ~FrameLoader();

    void start();
    void stop();
    void seek(int8_t loadDir, int64_t seekPts);
    RGBFrame* getFrame();
    void putFrame(RGBFrame* unusedFrame);
    void createFrames(size_t count, int w, int h);
};

struct FrameQueue {
    static constexpr int64_t capacity = 5;
    static constexpr int64_t deltaMin = 1;

    CircleBuffer<RGBFrame*, capacity, nullptr> items;
    int64_t selected = -1;
    int8_t loadDir = 1;

    const RGBFrame* curr();
    const RGBFrame* next();
    void print() const;
    void play(FrameLoader& loader);
    void seekNextFrame(FrameLoader& loader);
    void seekPrevFrame(FrameLoader& loader);
    void fillFrom(FrameLoader& loader);
    void flush(FrameLoader& loader);

private:
    bool tooFarFromBegin() const;
    bool tooFarFromEnd() const;
    void tryFillBack(FrameLoader& loader);
    void tryFillFront(FrameLoader& loader);
};
