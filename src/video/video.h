#pragma once 
#include "ffmpeg.h"
#include "image.h"

struct RGBConverter {
    SwsContext* swsContext = nullptr;
    uint8_t* destFrame[AV_NUM_DATA_POINTERS] = { nullptr };
    int destLineSize[AV_NUM_DATA_POINTERS] = { 0 };

    bool createContext(const AVCodecContext* decoder);
    void destroyContext();
    int toRGB(const AVFrame* frame, Image& result);
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
    bool nextFrame(Image& result);
    bool prevFrame(int64_t pts, Image& result);

private:
    bool readFrame() const;
    bool toRGB(const AVFrame* frame, Image& result);
};


