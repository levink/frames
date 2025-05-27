#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
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

SceneSize scene;
Render render;
VideoReader reader;
Image image;

static GLuint createTexture(Image& image) {

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
        image.width,            // Texture width
        image.height,		    // Texture height
        0,						// Border width
        GL_RGB,			        // Source format
        GL_UNSIGNED_BYTE,		// Source data type
        image.pixels);          // Source data pointer
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return videoTextureId;
}
static void updateTexture(GLuint textureId, const Image& image) {
    //todo: Probably better to use PBO for streaming data
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexSubImage2D(GL_TEXTURE_2D, // Target
        0,						// Mip-level
        0,                      // X-offset
        0,                      // Y-offset
        image.width,            // Texture width
        image.height,           // Texture height
        GL_RGB,			        // Source format
        GL_UNSIGNED_BYTE,		// Source data type
        image.pixels);          // Source data pointer
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
    else if (key.is(COMMA)) {
        reader.prevFrame(image.pts - 1, image);
        updateTexture(render.frame[0].textureId, image);
        updateTexture(render.frame[1].textureId, image);
    }
    else if (key.is(PERIOD)) {
        reader.nextFrame(image);
        updateTexture(render.frame[0].textureId, image);
        updateTexture(render.frame[1].textureId, image);
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
    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
    glfwSetFramebufferSizeCallback(window, reshape);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseClick);
    glfwSetCursorPosCallback(window, mouseMove);
    glfwSetScrollCallback(window, mouseScroll);
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

    // Pre-render step, for creating fonts & styles
    auto windowPadding = ImVec2(scene.windowPadding, scene.windowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

std::atomic<bool> eventLoopStop = false;
static void eventLoop() {
    using std::this_thread::sleep_for;
    using std::chrono::seconds;

    eventLoopStop.store(false);
    while (!eventLoopStop.load()) {
        sleep_for(seconds(1));
        std::cout << "tick" << std::endl;
    }
    std::cout << "stopped" << std::endl;
}

int main() {

    //std::thread t1 = std::thread([]() {
    //    eventLoop();
    //});
    
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
    
    image.allocate(reader.decoderContext->width, reader.decoderContext->height);

    if (!reader.nextFrame(image)) {
        std::cout << "File read - error" << std::endl;
        return -1;
    }

    GLuint textureId = createTexture(image);
    render.frame[0].textureId = textureId;
    render.frame[0].mesh.setSize(image.width, image.height);

    render.frame[1].textureId = textureId;
    render.frame[1].mesh.setSize(image.width, image.height);

    const auto& style = ImGui::GetStyle();
    while (!glfwWindowShouldClose(window)) {

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

            static float progress = 0.1f;
            float itemWidth = 0.5 * (workSize.x - 3 * style.WindowPadding.x);
            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::PushItemWidth(itemWidth);
            ImGui::SliderFloat("##slider1", &progress, 0.0f, 1.0f, "%.3f", flags);
            ImGui::SameLine(0.f, style.WindowPadding.x);
            ImGui::SliderFloat("##slider2", &progress, 0.0f, 1.0f, "%.3f", flags);
            ImGui::PopItemWidth();
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

    /*eventLoopStop.store(true);
    t1.join();*/
    
    ImGui::PopStyleVar(2);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    render.destroy();
    glfwTerminate();
	return 0;
}
