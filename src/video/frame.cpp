#include <cstdint>
#include <string>
#include "ffmpeg.h"
#include "frame.h"

static size_t getAlignedSize(int32_t lineSize, int32_t lineCount) {
    /*
        We use swscale library for converting images from YUV to RGB.
        Note that it is working with 32-byte aligned memory, because it is more efficient.
        So we must allocate a little bit more bytes, than (3 * w * h) for RGB images
    */
    int32_t tail = 32 - lineSize % 32;
    return static_cast<size_t>(lineSize) * lineCount + tail;
}

RGBFrame::RGBFrame(int32_t width, int32_t height) :
    width(width),
    height(height) {
    lineSize = av_image_get_linesize(AV_PIX_FMT_RGB24, width, 0);
    auto aligned = getAlignedSize(lineSize, height);
    pixels = new uint8_t[aligned];
}

RGBFrame::~RGBFrame() {
    delete[] pixels;
}

bool RGBFrame::checkSize(int w, int h) const {
    return w == width && h == height;
}
