#pragma once 
#include <vector>
#include <mutex>
#include <condition_variable>
#include "ffmpeg.h"
#include "frame.h"

struct RGBConverter {
    SwsContext* swsContext = nullptr;
    uint8_t* destFrame[AV_NUM_DATA_POINTERS] = { nullptr };
    int destLineSize[AV_NUM_DATA_POINTERS] = { 0 };

    bool createContext(const AVCodecContext* decoder);
    void destroyContext();
    int toRGB(const AVFrame* frame, RGBFrame& result);
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
    //int64_t toTS(long long micros) {

    //}
};

struct VideoReader {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* decoderContext = nullptr;
    const AVCodec* decoder = nullptr;
    int videoStreamIndex = -1;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    RGBConverter converter;

    VideoReader();
    ~VideoReader();

    bool openFile(const char* fileName);
    bool nextFrame(RGBFrame& result);
    bool prevFrame(int64_t pts, RGBFrame& result);
    StreamInfo getStreamInfo() const;

private:
    bool readFrame() const;
    bool toRGB(const AVFrame* frame, RGBFrame& result);
};

class FramePool {
    std::mutex mtx;
    std::vector<RGBFrame*> items;
    int frameWidth = 0;
    int frameHeight = 0;

public:
    FramePool() = default;
    ~FramePool();
    void createFrames(size_t count, int w, int h);
    void put(RGBFrame* item);
    RGBFrame* get();
};

struct PlayLoop {
    std::thread t;
    std::atomic<bool> finished = false;
    FramePool& framePool;
    VideoReader& reader;
    int64_t seek_pts = -1;

    PlayLoop(FramePool& pool, VideoReader& reader);
    ~PlayLoop();
    void start();
    void stop();

private:
    void playback();
    bool readFrame(RGBFrame* result);


private:
    std::mutex mtx;
    std::condition_variable cv;
    RGBFrame* nextFrame = nullptr;
    bool sendFrame(RGBFrame* newFrame);

public:
    RGBFrame* next();
    void seek(int64_t pts);
};

