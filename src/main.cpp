﻿#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include "image/lodepng.h"
#include "ui/ui.h"
#include "util/fpscounter.h"
#include "video/video.h"
#include "render.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::duration_cast;

static void seekProgress(float progress);
static void seekPts(int64_t pts);


struct SceneSize {
    /* Styles */
    int windowPadding = 4;

    /* Sizes depends on style only */
    int sliderHeight = 0;

    /* Main window */
    int windowWidth = 0;
    int windowHeight = 0;
    
    void updateStyleSizes() {
        const ImGuiStyle& style = ImGui::GetStyle();
        sliderHeight = ImGui::GetFontSize() +
            style.FramePadding.y * 2 +
            style.WindowPadding.y * 2;
    }
};

struct PlayState {
    bool paused = false;
    bool update = false;    // when paused or manual seek
    float progress = 0.f;   // [0; 100]
    int64_t framePts = 0;   // last seen frame pts 
    int64_t frameDur = 0;   // last seen frame duration
};

Render render;
SceneSize scene;

VideoReader reader;
FrameLoader loader(reader);
FrameQueue frameQ;
StreamInfo info;
PlayState ps;


struct SlideDetector {
    steady_clock::time_point last;
    bool move = false;
    bool hold = false;
    bool pausedPrev = false;
    
    void update(bool changed, bool active, float progress) {

        if (!hold && active) {
            hold = true;
            pausedPrev = ps.paused;
            ps.paused = true;
        }
        else if (hold && !active) {
            hold = false;
            ps.paused = pausedPrev;
        }

        auto now = steady_clock::now();
        auto dt = duration_cast<milliseconds>(now - last).count();
        if (changed && !move) {
            move = true;
            last = now;
            seekProgress(progress);
            //std::cout << "move start" << std::endl;
        }
        else if (changed && move && dt > 25) {
            last = now;
            seekProgress(progress); //todo: works slow, try faster
            //std::cout << "keep moving " << std::endl;
        }
        else if (!changed && move && dt > 50) {
            move = false;
            last = now;
            seekProgress(progress);
            //std::cout << "move stop" << std::endl;
        }
    }
} slider;


static GLuint createTexture(int width, int height) {

    GLuint videoTextureId = 0;
    glGenTextures(1, &videoTextureId);
    glBindTexture(GL_TEXTURE_2D, videoTextureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, // Target
        0,						// Mip-level
        GL_RGBA,			    // Texture format
        width,                  // Texture width
        height,		            // Texture height
        0,						// Border width
        GL_RGB,			        // Source format
        GL_UNSIGNED_BYTE,		// Source data type
        nullptr);               // Source data pointer
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return videoTextureId;
}
static void updateTexture(GLuint textureId, const RGBFrame& frame) {
    //todo: Probably better to use PBO for streaming data
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexSubImage2D(GL_TEXTURE_2D, // Target
        0,						// Mip-level
        0,                      // X-offset
        0,                      // Y-offset
        frame.width,            // Texture width
        frame.height,           // Texture height
        GL_RGB,			        // Source format
        GL_UNSIGNED_BYTE,		// Source data type
        frame.pixels);          // Source data pointer
    glBindTexture(GL_TEXTURE_2D, 0);
}
static int64_t progressToPts(float progress) {
    return (progress * info.durationPts) / 100.f;
}
static void seekProgress(float progress) {
    auto pts = progressToPts(progress);
    seekPts(pts);
}
static void seekPts(int64_t pts) {
    // flush frameQ
    frameQ.flush(loader);
    frameQ.loadDir = 1;

    // seek && flush loader
    loader.seek(1, pts);

    // update UI
    ps.update = true;
    ps.framePts = pts;
    ps.progress = info.calcProgress(pts);
}
static void reshapeScene(int w, int h) {
    
    scene.windowWidth = w;
    scene.windowHeight = h;

    const int fw = w / 2;
    const int fh = h - scene.sliderHeight;

    render.frame[0].leftTop = { 0, 0 };
    render.frame[0].size = { fw, fh };
    render.frame[0].vp = { 0, scene.windowHeight - fh, fw, fh };
    render.frame[0].cam.reshape(fw, fh);

    render.frame[1].leftTop = { fw, 0 };
    render.frame[1].size = { w - fw, fh };
    render.frame[1].vp = { fw, scene.windowHeight - fh, w - fw, fh };
    render.frame[1].cam.reshape(w - fw, fh);
}
static void reshape(GLFWwindow*, int w, int h) {
    reshapeScene(w, h);
}
static void keyCallback(GLFWwindow* window, int keyCode, int scanCode, int action, int mods) {
    using namespace ui::keyboard;
    auto key = KeyEvent(keyCode, action, mods);

    if (key.is(ESC)) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (key.is(R)) {
        std::cout << "Reload shaders" << std::endl;
        render.reloadShaders();
    }
    else if (key.is(SPACE)) {
        ps.paused = !ps.paused;
        ps.update = false;
        if (ps.paused) {
            frameQ.print();
        } else {
            frameQ.play(loader);
        }
    }
    else if (key.is(LEFT)) {
        if (ps.paused) {
            ps.update = true;
            frameQ.seekPrevFrame(loader);
            frameQ.print();
        }
        else {
            // minus 1 second
            auto oneSecond = info.MicrosToPts(1000000);
            auto pts = std::max(0LL, ps.framePts - oneSecond);
            seekPts(pts);
        }
    }
    else if (key.is(RIGHT)) {
        if (ps.paused) {
            ps.update = true;
            frameQ.seekNextFrame(loader);
            frameQ.print();
        }
        else {
            // plus 1 second
            auto oneSecond = info.MicrosToPts(1000000);
            auto pts = std::min(info.durationPts, ps.framePts + oneSecond);
            seekPts(pts);
        }
    }
}
static void mouseCallback(ui::mouse::MouseEvent event) {
    using namespace ui;
    using namespace ui::mouse;

    if (event.is(Action::MOVE, Button::LEFT)) {
        auto delta = event.getDelta();
        render.move(delta);
    }
    else if (event.is(Action::MOVE, Button::NO)) {
        auto cursor = event.getCursor();
        render.select(cursor);
    }
}
static void mouseClick(GLFWwindow*, int button, int action, int mods) {

    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    auto event = ui::mouse::click(button, action, mods);
    mouseCallback(event);
}
static void mouseMove(GLFWwindow* w, double x, double y) {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    auto mx = static_cast<int>(x);
    auto my = static_cast<int>(y);
    auto event = ui::mouse::move(mx, my);
    mouseCallback(event);
}
static void mouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    render.zoom(yoffset);
}
static bool loadGLES(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to load GLES2" << std::endl;
        return false;
    }
    
    printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    return true;
}
static void initWindow(GLFWwindow* window) {
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseClick);
    glfwSetCursorPosCallback(window, mouseMove);
    glfwSetScrollCallback(window, mouseScroll);
    glfwSetWindowPos(window, 400, 200);
    glfwSwapInterval(1);
    glfwSetTime(0.0);
}
static void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    //Default style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(scene.windowPadding, scene.windowPadding));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);

    // Pre-render step, for creating fonts & styles
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    scene.updateStyleSizes();
}
static void destroyImGui() {
    ImGui::PopStyleVar(2);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


/*
    todo:
        precompute PV_inverse matrix in FrameView
        draw points on video
        select between modes:
            1. move/scale video
            2. draw on video
            3. playing/steps (?)
        open file dialog - select file
*/
int main() {
    if (!glfwInit()) {
        std::cout << "glfwInit error" << std::endl;
        return -1;
    }

    int windowWidth = 1000;
    int windowHeight = 600;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Frames", nullptr, nullptr);
    if (!window) {
        std::cout << "glfwCreateWindow error" << std::endl;
        glfwTerminate();
        return -1;
    }
    if (!loadGLES(window)) {
        glfwTerminate();
        return -1;
    }
    render.loadShaders();

    initWindow(window);
    initImGui(window);
    reshapeScene(windowWidth, windowHeight);

    const char* fileName = "C:/Users/Konst/Desktop/k/IMG_3504.MOV";
    if (!reader.open(fileName)) {
        std::cout << "File open - error" << std::endl;
        return -1;
    }

    info = reader.getStreamInfo();
    int frameWidth = info.frameWidth;
    int frameHeight = info.frameHeight;

    GLuint textureId = createTexture(frameWidth, frameHeight);
    render.createFrame(0, textureId, frameWidth, frameHeight);
    render.createFrame(1, textureId, frameWidth, frameHeight);
    loader.createFrames(10, frameWidth, frameHeight);
    loader.start();

    FpsCounter fps;
    auto t1 = steady_clock::now();

    while (!glfwWindowShouldClose(window)) {

        auto now = steady_clock::now();

        frameQ.fillFrom(loader);
        {
            if (ps.paused && ps.update) {        
                const RGBFrame* frame = frameQ.curr();
                if (frame) {
                    ps.update = false;
                    ps.framePts = frame->pts;
                    ps.frameDur = frame->duration;
                    ps.progress = info.calcProgress(frame->pts);
                    updateTexture(render.frame[0].textureId, *frame);
                    updateTexture(render.frame[1].textureId, *frame);
                }
            }
            else if (!ps.paused) {

                auto duration = info.MicrosToPts(duration_cast<microseconds>(now - t1).count());
                if (duration > ps.frameDur || ps.update) {
                    const RGBFrame* frame = frameQ.next();
                    if (frame) {
                        auto deltaPts = duration - ps.frameDur;
                        if (deltaPts < frame->duration) {
                            t1 = now - microseconds(info.PtsToMicros(deltaPts));
                        } else {
                            t1 = now;
                        }

                        if (ps.update) {
                            ps.update = false;
                        }
                        else {
                            ps.framePts = frame->pts;
                            ps.frameDur = frame->duration;
                            ps.progress = info.calcProgress(frame->pts);
                        }
                        
                        updateTexture(render.frame[0].textureId, *frame);
                        updateTexture(render.frame[1].textureId, *frame);
                        
                        //fps.print();
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        const auto& workSize = main_viewport->WorkSize;
        ImGui::SetNextWindowPos(ImVec2(0, workSize.y - scene.sliderHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(workSize.x, scene.sliderHeight), ImGuiCond_Always);
        if (ImGui::Begin("SlidersWindow", nullptr, 
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoNavInputs | 
            ImGuiWindowFlags_NoNavFocus)) {

            const auto& style = ImGui::GetStyle();
            float itemWidth = 0.5 * (workSize.x - 3 * style.WindowPadding.x);
            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::PushItemWidth(itemWidth);
            bool changed1 = ImGui::SliderFloat("##slider1", &ps.progress, 0.0f, 100.0f, "%.f%%", flags);
            bool active1 = ImGui::IsItemActive();
            slider.update(changed1, active1, ps.progress);

            ImGui::SameLine(0.f, style.WindowPadding.x);

            bool changed2 = ImGui::SliderFloat("##slider2", &ps.progress, 0.0f, 100.0f, "%.f%%", flags);
            ImGui::PopItemWidth();

           /* if (progress != progress_old) {
                std::cout << "changed: " << changed1 << " " << changed2 << std::endl;
            }*/
            
        }        
        ImGui::End();
        //ImGui::ShowDemoWindow();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        render.draw();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glFlush();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    frameQ.flush(loader);
    loader.stop();
    render.destroy();
    destroyImGui();
    glfwTerminate();
	return 0;
}
