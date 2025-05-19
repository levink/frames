#pragma once 

#include "ffmpeg.h"

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
    int pixelsWidth = 0;
    int pixelsHeight = 0;

    FileReader();
    ~FileReader();

    bool openFile(const char* fileName);
    bool nextFrame();
    bool prevFrame();

private:
    bool readFrame() const;
    void processFrame(const AVFrame* frame);
};


