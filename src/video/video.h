#pragma once 
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "ffmpeg.h"
#include "frame.h"
#include "util/circlebuffer.h"

namespace video {
    
    typedef std::chrono::steady_clock::time_point time_point;

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
        void put(const std::vector<RGBFrame*>& frames);
        RGBFrame* get();
    };

    struct StreamInfo {
        AVRational time_base = { 0, 1 };
        int64_t durationPts = 0;
        int64_t framesCount = 0;
        int width = 0;
        int height = 0;
        float calcProgress(int64_t pts) const;
        int64_t ptsToMicros(int64_t pts) const;
        int64_t microsToPts(int64_t micros) const;
        int64_t progressToPts(float progress) const;
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
        int videoStreamIndex = -1;
        AVPacket* packet = nullptr;
        AVFrame* frame = nullptr;
        FrameConverter converter;
        bool eof = false;

        VideoReader();
        ~VideoReader();

        bool open(const char* fileName);
        bool read(RGBFrame& result, int64_t skipPts = 0);
        bool seek(int64_t pts);
        StreamInfo getStreamInfo() const;

    private:
        bool readRaw();
        bool convert(const AVFrame* frame, RGBFrame& result);
        void destroy();
    };

    class FrameLoader {
    public:
        static constexpr size_t cacheSize = 5;
    
    private:
        std::thread t;
        std::mutex mtx;
        std::condition_variable cv;
        std::atomic<bool> finished = false;
        FramePool pool;
        VideoReader reader;

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
        std::vector<RGBFrame*> prevCache;
        int64_t lastPts = -1;

        void playback();
        bool canWork();
        State copyState();
        void saveResult(RGBFrame* frame, State state);
        RGBFrame* readFrame(const State& state);

    public:
        FrameLoader() = default;
        ~FrameLoader();

        bool open(const char* fileName, StreamInfo& info);
        void start();
        void stop();
        void seek(int8_t loadDir, int64_t seekPts);
        RGBFrame* getFrame();
        void putFrame(RGBFrame* unusedFrame);
        void createFrames(size_t count, int w, int h);
    };

    struct FrameQueue {
        static constexpr size_t capacity = 10;
        static constexpr size_t deltaMin = 1;

        CircleBuffer<RGBFrame*, capacity, nullptr> items;
        size_t selected = 0;
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

    struct PlayState {
        bool started = false;
        bool hold = false;
        bool paused = true;
        bool update = true;     // flag for update frame when paused or manual seek
        float progress = 0.f;   // [0; 100]
        int64_t framePts = 0;   // last seen frame pts 
        int64_t frameDur = 0;   // last seen frame duration
    };

    struct Player {
        StreamInfo info;
        FrameLoader loader;
        FrameQueue frameQ;
        PlayState ps;
        time_point lastUpdate;

        bool start(const char* fileName);
        void stop();
        void seekProgress(float progress, bool hold);
        void seekLeft();
        void seekRight();
        void seekPts(int64_t pts);
        void pause(bool paused);
        bool hasUpdate(const time_point& now);
        const RGBFrame* currentFrame();
    };

}
