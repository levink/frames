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

struct PacketUnref {
    AVPacket* packet = nullptr;
    ~PacketUnref() {
        if (packet) {
            av_packet_unref(packet);
        }
    }
};

struct FileInfo {
    AVFormatContext* formatContext = nullptr;
    int videoIndex = -1;
    
    AVCodecContext* codecContext = nullptr;
    const AVCodec* codec;    

    SwsContext* swsContext = nullptr;
    uint8_t* pixelsRGBA = nullptr;
    int pixelsLineSize = 0;
    
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;  
    
    std::string error;
    bool openFile(const char* fileName);
    bool processFrame(AVFrame* frame);

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

        delete[] pixelsRGBA;
        pixelsRGBA = nullptr;
        pixelsLineSize = 0;
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

    auto width = codecContext->width;
    auto height = codecContext->height;
    swsContext = sws_getContext(
        width, height, codecContext->pix_fmt,
        width, height, AV_PIX_FMT_RGB0,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext) {
        error = "swsContext bad alloc";
        return false;
    }
    pixelsRGBA = new uint8_t[width * height * 4];
    pixelsLineSize = width * 4;

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

    bool finished = false;
    while (av_read_frame(formatContext, packet) >= 0 && !finished) {

        PacketUnref packetUnref{ packet };
        int response = avcodec_send_packet(codecContext, packetUnref.packet);
        if (packet->stream_index != videoIndex) {
            continue;
        }

        while (response >= 0) {
            response = avcodec_receive_frame(codecContext, frame);
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            }
            else if (response < 0) {
                error = "Error on receive frame: " + std::to_string(response);
                finished = true;
                break;
            }

            finished = processFrame(frame);
            if (finished) {
                break;
            }
        }
    }

    bool ok = (error == "");
    return ok;
}

int frameSkip = 100;
bool FileInfo::processFrame(AVFrame* frame) {

    if (frameSkip > 0) {
        frameSkip--;
        return false;
    }

    const auto width = frame->width;
    const auto height = frame->height;
    const auto lineSize = frame->linesize[0];

    int destLineSize[4] = { pixelsLineSize, 0, 0, 0 };
    uint8_t* dest[4] = { pixelsRGBA, nullptr, nullptr, nullptr };
    int ret = sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, dest, destLineSize);
    if (ret < 0) {
        std::cout << "processFrame: sws_scale() return " << ret << std::endl;
    }

    auto retPNG = lodepng::encode("./testRGB.png", pixelsRGBA, width, height, LodePNGColorType::LCT_RGBA);
    if (retPNG) {
        std::cout << "lodepng::encode() on rgb: Error code=" << retPNG << std::endl;
    }
    return true;
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
    bool ok = info.openFile("C:/Users/Konst/Desktop/k/IMG_3504.MOV");
    if (!ok) {
        std::cout << info.error << std::endl;
        return -1;
    }

	return 0;
}
