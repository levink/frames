#pragma once

struct RGBFrame {
    static constexpr int BYTES_PER_PIXEL = 3;
    int64_t pts = -1;
    int64_t duration = 0;
    int16_t width = 0;
    int16_t height = 0;
    size_t sizeInBytes = 0;
    uint8_t* pixels = nullptr;
    RGBFrame(int w, int h);
    ~RGBFrame();
    bool checkSize(int w, int h) const;
};