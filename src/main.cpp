#include <iostream>
#include <GLFW/glfw3.h>
#include "ffmpeg.h"
#include "lodepng.h"

namespace png {
    static void saveGrayPng(const char* fileName, const AVFrame* frame) {
        const auto width = frame->width;
        const auto height = frame->height;
        const auto lineSize = frame->linesize[0];

        std::vector<uint8_t> pixelsGray(width * height * 4, 0);
        uint8_t* lineBegin = frame->data[0];
        for (int y = 0; y < frame->height; y++) {
            for (int x = 0; x < frame->width; x++) {
                uint8_t gray = lineBegin[x];

                auto base = (y * width + x) * 4;
                auto rIndex = base + 0;
                auto gIndex = base + 1;
                auto bIndex = base + 2;
                auto aIndex = base + 3;

                pixelsGray[rIndex] = gray;
                pixelsGray[gIndex] = gray;
                pixelsGray[bIndex] = gray;
                pixelsGray[aIndex] = 255;
            }
            lineBegin += lineSize;
        }

        unsigned retPNG = lodepng::encode(fileName, pixelsGray, width, height, LodePNGColorType::LCT_RGBA);
        if (retPNG) {
            std::cout << "lodepng::encode() on gray: Error code=" << retPNG << std::endl;
        }
    }
}

struct FileInfo {
    AVFormatContext* formatContext = nullptr;
    AVCodecContext* codecContext = nullptr;
    SwsContext* swsContext = nullptr;
    int videoIndex = -1;
    const AVCodec* codec = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    
    std::string error;
    bool openFile(const char* fileName);
    void processFrame(AVFrame* frame);

    ~FileInfo() {
        if (formatContext) {
            avformat_free_context(formatContext);
        }
        if (codecContext) {
            avcodec_free_context(&codecContext);
        }
        if (swsContext) {
            sws_freeContext(swsContext);
        }
        if (packet) {
            av_packet_free(&packet);
        }
        if (frame) {
            av_frame_free(&frame);
        }        
    }
};

bool FileInfo::openFile(const char* fileName) {

    formatContext = avformat_alloc_context();
    if (!formatContext) {
        error = "formatContext bad alloc";
        return false;
    }

    int ret = avformat_open_input(&formatContext, fileName, nullptr, nullptr);
    if (ret < 0) {
        error = "Cannot open input file";
        return false;
    }

    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) {
        error = "Stream info not found";
        return false;
    }

    ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (ret < 0) {
        error = "Video stream not found";
        return false;
    } 
    videoIndex = ret;

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        error = "codecContext bad alloc";
        return false;
    }

    const auto& stream = formatContext->streams[videoIndex];
    ret = avcodec_parameters_to_context(codecContext, stream->codecpar);
    if (ret < 0) {
        error = "Bad copy params from stream to codec context";
        return false;
    }

    swsContext = sws_getContext(
        codecContext->width, codecContext->height, codecContext->pix_fmt,
        codecContext->width, codecContext->height, AV_PIX_FMT_RGB0,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext) {
        error = "swsContext bad alloc";
        return false;
    }

    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret < 0) {
        error = "Cannot open video decoder";
        return false;
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();
    if (!packet || !frame) {
        error = "Packet or frame bad alloc";
        return false;
    }

    int frameSkip = 100;
    int frameReads = 101;
    while (av_read_frame(formatContext, packet) >= 0 && frameReads > 0) {
        int response = avcodec_send_packet(codecContext, packet);
        if (packet->stream_index != videoIndex) {
            av_packet_unref(packet);
            continue;
        }

        while (response >= 0) {
            response = avcodec_receive_frame(codecContext, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            }
            else if (response < 0) {
                error = "Error on receive frame: " + std::to_string(response);
                return false;
            }

            frameReads--;
            if (frameSkip > 0) {
                frameSkip--;
            }
            else {
                processFrame(frame);
            }
        }
        av_packet_unref(packet);
    }

    return true;
}
void FileInfo::processFrame(AVFrame* frame) {

    const auto width = frame->width;
    const auto height = frame->height;
    const auto lineSize = frame->linesize[0];

    int destLineSize[4] = { width * 4, 0, 0, 0 };           // *3 maybe?
    uint8_t* pixelsRGBA = new uint8_t[width * height * 4];  // *3 maybe?
    uint8_t* dest[4] = { pixelsRGBA, nullptr, nullptr, nullptr };
    int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, dest, destLineSize);
    if (ret < 0) {
        std::cout << "sws_scale() return " << ret << std::endl;
    }

    auto retPNG = lodepng::encode("./testRGB.png", pixelsRGBA, width, height, LodePNGColorType::LCT_RGBA);
    if (retPNG) {
        std::cout << "lodepng::encode() on rgb: Error code=" << retPNG << std::endl;
    }
}

static void savePNG() {

    constexpr uint32_t SIZE = 32;
    std::vector<uint8_t> pixels(SIZE * SIZE * 4, 0);

    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {

            auto base  = (y * SIZE + x) * 4;
            auto red   = base + 0;
            auto green = base + 1;
            auto blue  = base + 2;
            auto alpha = base + 3;

            pixels[red]   = 255;
            pixels[green] = 255;
            pixels[blue]  = 0;
            pixels[alpha] = 255;
        }
    }

    unsigned err = lodepng::encode("./test.png", pixels, SIZE, SIZE, LodePNGColorType::LCT_RGBA);
    if (err) {
        std::cout << "lodepng::encode(): Error code=" << err << std::endl;
    }
}

int main() {

	/*if (!glfwInit()) {
		std::cout << "glfwInit - error. Exit." << std::endl;
		return -1;
	} else {
		std::cout << "glfwInit - ok" << std::endl;
	}*/

    av_log_set_level(AV_LOG_FATAL);
   
    FileInfo info;
    bool opened = info.openFile("C:/Users/Konst/Desktop/k/IMG_3504.MOV");
    if (!opened) {
        std::cout << info.error << std::endl;
        return -1;
    }

	return 0;
}
