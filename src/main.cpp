#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include "image/lodepng.h"
#include "ui/ui.h"
#include "video/video.h"
#include "render.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

struct SceneSize {
    
    /* Main window */
    int windowWidth = 0;
    int windowHeight = 0;
    int windowPadding = 4;
    int sliderHeight = 0;

    /* Render */
    int left = 0;
    int bottom = 0;
    int width = 0;
    int height = 0;
    
    void reshape(int w, int h) {
        windowWidth = w;
        windowHeight = h;

        const ImGuiStyle& style = ImGui::GetStyle();
        sliderHeight = ImGui::GetFontSize() +
            style.FramePadding.y * 2 +
            style.WindowPadding.y * 2;

        left = 0;
        bottom = sliderHeight;
        width = windowWidth;
        height = windowHeight - sliderHeight;
    }
};

struct PlayState {
    float progress = 0.f;
    int64_t pts = 0;
    int64_t dur = 0;

    bool paused = false;
    int nextFrame = 0; // when paused
};

Render render;
SceneSize scene;
VideoReader reader;
FramePool framePool;
PlayLoop playLoop(framePool, reader);
PlayState ps;

struct FrameQueue {
    const size_t capacity = 5;
    const size_t deltaMin = 1;
    std::deque<RGBFrame*> items;
    size_t selected = -1;
    int8_t loadDir = 1;

    const RGBFrame* next() {
       
        if (items.empty() || selected + 1 == items.size()) {
            return nullptr;
        }

        selected++;
        return items[selected];
    }    

    const RGBFrame* prev() {
        constexpr int first = 0;
        if (items.empty() || selected == first) {
            return nullptr;
        }
        
        selected--;
        return items[selected];
    }

    void print() const {
        std::cout << "[";
        for (size_t i = 0; i < items.size(); i++) {
            auto elem = items[i];
            char mark = (i == selected) ? '=' : ' ';
            std::cout << elem->pts << mark << " ";
        }
        std::cout << "] ";

        if (!items.empty()) {
            std::cout << "pts=" << items[selected]->pts;
        }

        std::cout << std::endl;
    }

    void seekNext() {
        if (tooFarFromEnd()) {
            return;
        }

        if (loadDir < 0 && !items.empty()) {
            loadDir = 1;
            auto last = items.back();
            auto seek = last->pts + last->duration;
            playLoop.set(loadDir, seek);
        }
    }

    void seekPrev() {
        if (tooFarFromBegin()) {
            return;
        }

        if (loadDir > 0 && !items.empty()) {
            loadDir = -1;
            auto first = items.front();
            auto seek = first->pts - 1;
            playLoop.set(loadDir, seek);
        }
    }

    void tryFill(PlayLoop& frameProvider) {
        if (loadDir > 0) {
            tryFillBack(frameProvider);
            return;
        }
        if (loadDir < 0) {
            tryFillFront(frameProvider);
            return;
        } 
    }

private:
    bool tooFarFromBegin() const {
        return selected > deltaMin;
    }
    bool tooFarFromEnd() const {
        return selected + deltaMin + 1 < items.size();
    }
    void tryFillBack(PlayLoop& frameProvider) {
        if (tooFarFromEnd()) {
            return;
        }

        auto frame = frameProvider.next();
        if (!frame) {
            return;
        }

        bool added = pushBack(frame);
        if (!added) {
            frameProvider.putUnused(frame);
            return;
        }

        auto front = popFront();
        frameProvider.putUnused(front);
    }
    void tryFillFront(PlayLoop& frameProvider) {
        if (tooFarFromBegin()) {
            return;
        }

        auto frame = frameProvider.next();
        if (!frame) {
            return;
        }

        bool added = pushFront(frame);
        if (!added) {
            frameProvider.putUnused(frame);
            return;
        }

        auto back = popBack();
        frameProvider.putUnused(back);
    }
    bool pushBack(RGBFrame* frame) {
        if (items.empty()) {
            items.push_back(frame);
            return true;
        }

        auto back = items.back();
        auto backPts = back->pts + back->duration;
        if (backPts == frame->pts) {
            items.push_back(frame);
            return true;
        }

        //bad pts, skip frame
        return false;
    }
    bool pushFront(RGBFrame* frame) {
        if (items.empty()) {
            items.push_front(frame);
            selected++;
            return true;
        }

        auto front = items.front();
        auto framePts = frame->pts + frame->duration;
        if (framePts == front->pts) {
            items.push_front(frame);
            selected++;
            return true;
        }

        //bad pts, skip frame
        return false;
        
    }
    RGBFrame* popBack() {
        bool tooBigQueue = items.size() > capacity;
        bool lastIsNotUsed = selected < items.size() - 1;
        if (tooBigQueue && lastIsNotUsed) {
            auto back = items.back();
            items.pop_back();
            return back;
        }
        return nullptr;
    }
    RGBFrame* popFront() {
        bool tooBigQueue = items.size() > capacity;
        bool firstIsNotUsed = selected > 0;
        if (tooBigQueue && firstIsNotUsed) {
            auto front = items.front();
            items.pop_front();
            selected--;
            return front;
        }
        return nullptr;
    }
} frameQ;


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
static void reshapeScene(int w, int h) {
    scene.reshape(w, h);
    render.reshape(scene.left, scene.bottom, scene.width, scene.height);
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
        ps.nextFrame = 0;
        
        if (ps.paused) {    
            frameQ.print();
        } else {
            if (frameQ.loadDir == -1) {
                frameQ.loadDir = 1;
                if (!frameQ.items.empty()) {
                    auto last = frameQ.items.back();
                    auto seek = last->pts + last->duration;
                    playLoop.set(frameQ.loadDir, seek);
                }
            }
            
        }
    }
    else if (key.is(LEFT)) {
        if (ps.paused) {
            ps.nextFrame = -1;
            frameQ.seekPrev();
        }
    }
    else if (key.is(RIGHT)) {
        if (ps.paused) {
            ps.nextFrame = 1;
            frameQ.seekNext();
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
    auto my = static_cast<int>(scene.windowHeight - y);
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
}
static void destroyImGui() {
    ImGui::PopStyleVar(2);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

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
    StreamInfo videoInfo = reader.getStreamInfo();

    int frameWidth = reader.decoderContext->width;
    int frameHeight = reader.decoderContext->height;

    GLuint textureId = createTexture(frameWidth, frameHeight);
    render.createFrame(0, textureId, frameWidth, frameHeight);
    render.createFrame(1, textureId, frameWidth, frameHeight);
    framePool.createFrames(10, frameWidth, frameHeight);
    playLoop.start();

    /*
    auto fps_t1 = steady_clock::now();
    auto fps_count = 0;
    fps_count++;
        auto fps_t2 = steady_clock::now();
        auto dd = duration_cast<microseconds>(fps_t2 - fps_t1).count();
        if (dd > 1000000) {
            std::cout << fps_count << std::endl;
            fps_t1 = steady_clock::now();
            fps_count = 0;
    }*/

    using std::chrono::microseconds;
    using std::chrono::duration_cast;
    using std::chrono::steady_clock;
    auto t1 = steady_clock::now();

    while (!glfwWindowShouldClose(window)) {

        {
            frameQ.tryFill(playLoop);

            if (ps.paused) {    
                const RGBFrame* frame =
                    ps.nextFrame > 0 ? frameQ.next() :
                    ps.nextFrame < 0 ? frameQ.prev() :
                    nullptr;

                if (frame) {
                    frameQ.print();
                    ps.pts = frame->pts;
                    ps.dur = frame->duration;
                    ps.progress = videoInfo.calcProgress(frame->pts);
                    ps.nextFrame = 0;

                    updateTexture(render.frame[0].textureId, *frame);
                    updateTexture(render.frame[1].textureId, *frame);
                }
            }
            else {
                // TODO: this code is for 30 fps. Need more general solution based on (pts + dur)
                auto t2 = steady_clock::now();
                auto dt = duration_cast<microseconds>(t2 - t1).count();
                if (1000000 <= dt * 30) {
                    t1 = t2;

                    const RGBFrame* frame = frameQ.next();
                    if (frame) {
                        ps.pts = frame->pts;
                        ps.dur = frame->duration;
                        ps.progress = videoInfo.calcProgress(frame->pts);
                        ps.nextFrame = 0;

                        updateTexture(render.frame[0].textureId, *frame);
                        updateTexture(render.frame[1].textureId, *frame);
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

            /*static float progress = player.progress;*/
            const auto& style = ImGui::GetStyle();
            float itemWidth = 0.5 * (workSize.x - 3 * style.WindowPadding.x);
            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::PushItemWidth(itemWidth);
            bool changed1 = ImGui::SliderFloat("##slider1", &ps.progress, 0.0f, 100.0f, "%.f%%", flags);
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

    playLoop.stop();
    render.destroy();
    destroyImGui();
    glfwTerminate();
	return 0;
}
