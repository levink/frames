#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
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

    /* Playback */
    bool paused = false;
    
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

SceneSize scene;
Render render;
VideoReader reader;

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
        scene.paused = !scene.paused;
    }
    else if (key.is(COMMA)) {
        //reader.prevFrame(frameRGB.pts - 1, frameRGB);
       /* updateTexture(render.frame[0].textureId, frameRGB);
        updateTexture(render.frame[1].textureId, frameRGB);*/
    }
    else if (key.is(PERIOD)) {
        //reader.nextFrame(frameRGB);
        /*updateTexture(render.frame[0].textureId, frameRGB);
        updateTexture(render.frame[1].textureId, frameRGB);*/
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
    //glfwSwapInterval(1);
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

    FramePool framePool;
    FrameChannel frameChannel;
    PlayLoop playLoop(framePool, frameChannel, reader);

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
    if (!reader.openFile(fileName)) {
        std::cout << "File open - error" << std::endl;
        return -1;
    }

    int fileWidth = reader.decoderContext->width;
    int fileHeight = reader.decoderContext->height;

    GLuint textureId = createTexture(fileWidth, fileHeight);
    render.createFrame(0, textureId, fileWidth, fileHeight);
    render.createFrame(1, textureId, fileWidth, fileHeight);

    framePool.createFrames(3, fileWidth, fileHeight);
    playLoop.start();



    using namespace std::chrono_literals;
    using std::chrono::microseconds;
    using std::chrono::duration_cast;
    using std::chrono::steady_clock;

    auto t1 = steady_clock::now();
    auto fps_t1 = steady_clock::now();
    auto fps_count = 0;

    float videoProgress = 0.f;
    StreamInfo videoInfo = reader.getStreamInfo();

    while (!glfwWindowShouldClose(window)) {

        {
            auto t2 = steady_clock::now();
            auto durationMicros = duration_cast<microseconds>(t2 - t1).count();

            if (durationMicros * 30ms >= 1000000ms ) { // TODO: this code for 30 fps. Need more general solution
                t1 = t2;

                if (!scene.paused) {
                    RGBFrame* frame = frameChannel.get();
                    if (frame) {
                        //scene.paused = true;
                        updateTexture(render.frame[0].textureId, *frame);
                        updateTexture(render.frame[1].textureId, *frame);
                        framePool.put(frame);

                        int64_t pts = frame->pts;
                        int64_t dur = videoInfo.duration;
                        videoProgress = (pts * 100.f) / dur;
                    }
                }

                fps_count++;
                auto fps_t2 = steady_clock::now();
                auto dd = duration_cast<microseconds>(fps_t2 - fps_t1).count();
                if (dd > 1000000) {
                    std::cout << fps_count << std::endl;
                    fps_t1 = steady_clock::now();
                    fps_count = 0;
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

            static float progress = videoProgress;

            float progress_old = progress;
            const auto& style = ImGui::GetStyle();
            float itemWidth = 0.5 * (workSize.x - 3 * style.WindowPadding.x);
            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::PushItemWidth(itemWidth);
            bool changed1 = ImGui::SliderFloat("##slider1", &videoProgress, 0.0f, 100.0f, "%.f%%", flags);
            ImGui::SameLine(0.f, style.WindowPadding.x);
            bool changed2 = ImGui::SliderFloat("##slider2", &videoProgress, 0.0f, 100.0f, "%.2f", flags);
            ImGui::PopItemWidth();

            if (progress != progress_old) {
                std::cout << "changed: " << changed1 << " " << changed2 << std::endl;
            }
            
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
