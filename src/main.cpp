#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include "image/lodepng.h"
#include "ui/ui.h"
#include "video/video.h"
#include "render.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

FileReader file;
Render render;

static void updateTexture(GLuint textureId, const FileReader& reader) {
    //todo: 
    // 1. glTexSubImage2D is probably better for update than glTexImage2D
    // 2. or use PBO for streaming
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, // Target
        0,						// Mip-level
        GL_RGBA,			    // Texture format
        reader.pixelsWidth,     // Texture width
        reader.pixelsHeight,    // Texture height
        0,						// Border width
        GL_RGB,			        // Source format
        GL_UNSIGNED_BYTE,		// Source data type
        reader.pixelsRGB);      // Source data pointer
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void reshape(GLFWwindow*, int w, int h) {
    //todo: finish this
    //render.reshape(w, h);
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
        const auto& textureId = render.shaders.video.videoTextureId;
        updateTexture(textureId, file);
    }
    else if (key.is(PERIOD)) {
        file.nextFrame();
        const auto& textureId = render.shaders.video.videoTextureId;
        updateTexture(textureId, file);

        /*using namespace std::chrono;
        auto t0 = high_resolution_clock::now();
        auto t1 = high_resolution_clock::now();
        auto t2 = high_resolution_clock::now();
        auto delta1 = std::chrono::duration_cast<milliseconds>(t1 - t0);
        auto delta2 = std::chrono::duration_cast<milliseconds>(t2 - t1);
        std::cout << "delta1=" << delta1 << " delta2=" << delta2 << std::endl;*/
    }
}
static void mouseCallback(ui::mouse::MouseEvent event) {
    using namespace ui;
    using namespace ui::mouse;
    if (event.is(Action::MOVE, Button::LEFT)) {
        auto delta = event.getDelta();
        render.mesh.move(delta.x, -delta.y);
    }
}
static void mouseClick(GLFWwindow*, int button, int action, int mods) {
    auto event = ui::mouse::click(button, action, mods);
    mouseCallback(event);
}
static void mouseMove(GLFWwindow*, double x, double y) {
    auto mx = static_cast<int>(x);
    auto my = static_cast<int>(y);
    auto event = ui::mouse::move(mx, my);
    mouseCallback(event);
}
static void mouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
    float value = yoffset * 0.1;
    render.mesh.zoom(value);
    //std::cout << render.mesh.scale.x << std::endl;
}

int main() {
    if (!glfwInit()) {
        std::cout << "glfwInit error" << std::endl;
        return -1;
    }
    
    constexpr int windowWidth = 800;
    constexpr int windowHeight = 600;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Frames", nullptr, nullptr);
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
    }
    else {
        printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
    }
    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseClick);
    glfwSetCursorPosCallback(window, mouseMove);
    glfwSetScrollCallback(window, mouseScroll);
    glfwSwapInterval(1);
    glfwSetTime(0.0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    // Pre-render step, for creating fonts & styles
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    const char* fileName = "C:/Users/Konst/Desktop/k/IMG_3504.MOV";
    if (!file.openFile(fileName)) {
        std::cout << "File open - error" << std::endl;
        return -1;
    }
    if (!file.nextFrame()) {
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
        glTexImage2D(GL_TEXTURE_2D, // Target
            0,						// Mip-level
            GL_RGBA,			    // Texture format
            file.pixelsWidth,       // Texture width
            file.pixelsHeight,		// Texture height
            0,						// Border width
            GL_RGB,			        // Source format
            GL_UNSIGNED_BYTE,		// Source data type
            file.pixelsRGB);        // Source data pointer
        glBindTexture(GL_TEXTURE_2D, 0);
        render.shaders.video.videoTextureId = videoTextureId;
        render.mesh.setSize(file.pixelsWidth, file.pixelsHeight);
    }


    const auto& style = ImGui::GetStyle();
    const auto sliderHeight = ImGui::GetFontSize() + style.FramePadding.y * 2 + style.WindowPadding.y * 2;



    render.camera.reshape(0, sliderHeight, windowWidth, windowHeight - sliderHeight);
    render.loadResources();
    render.initResources();

    while (!glfwWindowShouldClose(window)) {

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        const auto& workPos = main_viewport->WorkPos;
        const auto& workSize = main_viewport->WorkSize;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::SetNextWindowPos(ImVec2(workPos.x, workSize.y - sliderHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(workSize.x, 0), ImGuiCond_Always);
        if (ImGui::Begin("SliderWindow", nullptr, 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoNavInputs | 
            ImGuiWindowFlags_NoNavFocus)) {

            static float progress = 0.1f;
            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::PushItemWidth(-FLT_MIN);
            ImGui::SliderFloat("##slider", &progress, 0.0f, 1.0f, "%.3f", flags);
            ImGui::PopItemWidth();
        }        
        ImGui::End();
        ImGui::PopStyleVar();

        //ImGui::ShowDemoWindow();

        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        render.draw();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
