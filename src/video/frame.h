#pragma once

struct RGBFrame {
    int32_t width = 0;
    int32_t height = 0;
    int32_t lineSize = 0;
    uint8_t* pixels = nullptr;
    int64_t pts = -1;
    int64_t dur = 0;

    RGBFrame(int32_t width, int32_t heigth);
    ~RGBFrame();
    bool checkSize(int w, int h) const;
};