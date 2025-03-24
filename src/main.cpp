#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "image/lodepng.h"
#include "video/ffmpeg.h"
#include "ui/ui.h"
#include "render.h"


Render render;

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
    const AVCodec* codec = nullptr;    

    SwsContext* swsContext = nullptr;
    uint8_t* pixelsRGBA = nullptr;
    int pixelsLineSize = 0;
    
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;  
    
    std::string error;
    bool openFile(const char* fileName);

    int frameSkip = 100; //todo: for debug
    bool processFrame(AVFrame* frame);

    FileInfo() {
        av_log_set_level(AV_LOG_FATAL);
    }

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

static void reshape(GLFWwindow*, int w, int h) {
    render.reshape(w, h);
}
static void keyCallback(GLFWwindow* window, int key, int scanCode, int action, int mods) {
    using namespace ui::keyboard;
    auto keyEvent = KeyEvent(key, action, mods);

    if (keyEvent.is(ESC)) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (keyEvent.is(R)) {
        std::cout << "Reload shaders" << std::endl;
        render.reloadShaders();
    }
   /* else if (keyEvent.is(B)) {
        std::cout << "Scene rebuild" << std::endl;
        render.scene.rebuild();
    }*/
}
/*void mouseCallback(ui::mouse::MouseEvent event) {
   using namespace ui;
    using namespace ui::mouse;

    auto cursor = event.getCursor();
    cursor.y = (float)render.camera.viewSize.y - cursor.y;
    auto& scene = render.scene;

    if (event.is(Action::PRESS, Button::LEFT)) scene.selectPoint(cursor);
    else if (event.is(Action::PRESS, Button::RIGHT)) scene.addPoint(cursor);
    else if (event.is(Action::MOVE, Button::LEFT))  scene.movePoint(cursor);
    else if (event.is(Action::RELEASE, Button::LEFT)) scene.recover();
} */
static void mouseClick(GLFWwindow*, int button, int action, int mods) {
  /*  auto event = ui::mouse::click(button, action, mods);
    mouseCallback(event);*/
}
static void mouseMove(GLFWwindow*, double x, double y) {
    /*auto mx = static_cast<int>(x);
    auto my = static_cast<int>(y);
    auto event = ui::mouse::move(mx, my);
    mouseCallback(event);*/
}

int main() {

    if (!glfwInit()) {
        std::cout << "glfwInit error" << std::endl;
        return -1;
    }
    
    constexpr int sceneWidth = 800;
    constexpr int sceneHeight = 600;
    GLFWwindow* window = glfwCreateWindow(sceneWidth, sceneHeight, "Frames", nullptr, nullptr);
    if (!window) {
        std::cout << "glfwCreateWindow error" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init glad" << std::endl;
        return -1;
    } else {
        printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    }

    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseClick);
    glfwSetCursorPosCallback(window, mouseMove);
    glfwSwapInterval(1);
    glfwSetTime(0.0);

   /* FileInfo info;
    bool ok = info.openFile("C:/Users/Konst/Desktop/k/IMG_3504.MOV");
    if (!ok) {
        std::cout << info.error << std::endl;
        return -1;
    }*/


    render.reshape(sceneWidth, sceneHeight);
    render.loadResources();
    render.initResources();
    render.videoMesh.position = {
        { 100, 100 },
        { 500, 100 },
        { 500, 300 },
        { 100, 300 }
    };
    render.videoMesh.face = {
        { 0, 1, 2 },
        { 2, 3, 0 }
    };

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        render.draw();

        glFlush();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    render.destroy();
    glfwTerminate();
	return 0;
}
