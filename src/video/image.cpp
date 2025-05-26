#include "image.h"

Image::~Image() {
    delete[] pixels;
}

void Image::allocate(int w, int h) {
    delete[] pixels;
    pixels = new uint8_t[3 * w * h];
    width = static_cast<int16_t>(w);
    height = static_cast<int16_t>(h);
}

bool Image::checkSize(int w, int h) const {
    bool ready = 
        (w == width) && (h == height) ||
        (w == height) && (h == width);
    return ready;
}
