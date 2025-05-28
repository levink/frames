#include "frame.h"

RGBFrame::RGBFrame(int w, int h) {
    if (w > 0 && h > 0) {
        pixels = new uint8_t[3 * w * h];
        width = static_cast<int16_t>(w);
        height = static_cast<int16_t>(h);
    }
}

RGBFrame::~RGBFrame() {
    delete[] pixels;
}

bool RGBFrame::checkSize(int w, int h) const {
    bool ready = 
        (w == width) && (h == height) ||
        (w == height) && (h == width);
    return ready;
}
