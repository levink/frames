#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include "image/lodepng.h"
#include "video/video.h"
#include "ui/ui.h"
#include "render.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

FileReader file;
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

static void reshape(GLFWwindow*, int w, int h) {
    render.reshape(w, h);
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
    else if (key.is(COMMA)) {
        file.prevFrame();

        const auto& width = file.decoderContext->width;
        const auto& height = file.decoderContext->height;
        const auto& pixelsRGB = file.pixelsRGB;
        const auto& textureId = render.shaders.video.videoTextureId;
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, // Target
            0,						// Mip-level
            GL_RGBA,			    // Texture format
            width,                  // Texture width
            height,		            // Texture height
            0,						// Border width
            GL_RGB,			        // Source format
            GL_UNSIGNED_BYTE,		// Source data type
            pixelsRGB);             // Source data pointer
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else if (key.is(PERIOD)) {
        //using namespace std::chrono;
        //auto t0 = high_resolution_clock::now();
        file.nextFrame();

        //auto t1 = high_resolution_clock::now();
        const auto& width = file.decoderContext->width;
        const auto& height = file.decoderContext->height;
        const auto& pixelsRGB = file.pixelsRGB;
        const auto& textureId = render.shaders.video.videoTextureId;
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexImage2D(GL_TEXTURE_2D, // Target
            0,						// Mip-level
            GL_RGBA,			    // Texture format
            width,                  // Texture width
            height,		            // Texture height
            0,						// Border width
            GL_RGB,			        // Source format
            GL_UNSIGNED_BYTE,		// Source data type
            pixelsRGB);             // Source data pointer
        glBindTexture(GL_TEXTURE_2D, 0);

        /*auto t2 = high_resolution_clock::now();
        auto delta1 = std::chrono::duration_cast<milliseconds>(t1 - t0);
        auto delta2 = std::chrono::duration_cast<milliseconds>(t2 - t1);*/
        //std::cout << "delta1=" << delta1 << " delta2=" << delta2 << std::endl;
    }
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

    glfwSetWindowPos(window, 600, 100);
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init glad" << std::endl;
        return -1;
    }
    else {
        printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    }
    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseClick);
    glfwSetCursorPosCallback(window, mouseMove);
    glfwSwapInterval(1);
    glfwSetTime(0.0);


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // IF using Docking Branch
    ImGui_ImplGlfw_InitForOpenGL(window, true);             // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    const char* fileName = "C:/Users/Konst/Desktop/k/IMG_3504.MOV";
    if (file.open(fileName)) {
        std::cout << "File open - ok" << std::endl;
    } else {
        std::cout << "File open - error" << std::endl;
        return -1;
    }
   
    if (file.nextFrame()) {
        //std::cout << "File read - ok" << std::endl;
    } else {
        std::cout << "File read - error" << std::endl;
        return -1;
    }

    {
        /*std::vector<uint8_t> pixels;
        unsigned width = 0;
        unsigned height = 0;
        auto err = lodepng::decode(pixels, width, height, "../../data/img/cat.png", LodePNGColorType::LCT_RGBA);*/

        GLuint videoTextureId = 0;
        glGenTextures(1, &videoTextureId);
        glBindTexture(GL_TEXTURE_2D, videoTextureId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        const auto& width = file.decoderContext->width;
        const auto& height = file.decoderContext->height;
        const auto& pixelsRGB = file.pixelsRGB;
        glTexImage2D(GL_TEXTURE_2D, // Target
            0,						// Mip-level
            GL_RGBA,			    // Texture format
            width,                  // Texture width
            height,		            // Texture height
            0,						// Border width
            GL_RGB,			        // Source format
            GL_UNSIGNED_BYTE,		// Source data type
            pixelsRGB);             // Source data pointer
        glBindTexture(GL_TEXTURE_2D, 0);
        render.shaders.video.videoTextureId = videoTextureId;
    }
    

    render.reshape(sceneWidth, sceneHeight);
    render.loadResources();
    render.initResources();
    render.videoMesh.position = {
        { 0,   0   },
        { 500, 0   },
        { 500, 500 },
        { 0,   500 }
    };
    render.videoMesh.texture = {
        {0, 1},
        {1, 1},
        {1, 0},
        {0, 0}
    };
    render.videoMesh.face = {
        { 0, 1, 2 },
        { 2, 3, 0 }
    };

    while (!glfwWindowShouldClose(window)) {

      /*  ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();*/

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        const auto& workPos = main_viewport->WorkPos;
        const auto& workSize = main_viewport->WorkSize;

        const auto& style = ImGui::GetStyle();
        auto sliderHeight = ImGui::GetFontSize() + style.FramePadding.y * 2 + style.WindowPadding.y * 2;

        /*ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);*/
        //ImGui::SetNextWindowPos(ImVec2(workPos.x, workSize.y - sliderHeight), ImGuiCond_Once);
        //ImGui::SetNextWindowSize(ImVec2(workSize.x, 0), ImGuiCond_Once);
        //if (ImGui::Begin("SliderWindow", nullptr, ImGuiWindowFlags_NoDecoration)) {
        //    static float progress = 0.1f;
        //    ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

        //    ImGui::PushItemWidth(-FLT_MIN);
        //    ImGui::SliderFloat("##slider", &progress, 0.0f, 1.0f, "%.3f", flags);
        //    ImGui::PopItemWidth();
        //}        
        //ImGui::End();

        //ImGui::ShowDemoWindow();

        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        render.draw();

      /*  ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());*/

        glFlush();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    render.destroy();
    glfwTerminate();
	return 0;
}
