#pragma once

struct Image {
    int64_t pts = 0;
    int64_t duration = 0;
    int16_t width = 0;
    int16_t height = 0;
    uint8_t* pixels = nullptr;
    ~Image();
    void allocate(int w, int h);
    bool checkSize(int w, int h) const;
};