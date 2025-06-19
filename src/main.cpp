#include <glad/glad.h>
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

using std::cout;
using std::endl;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::duration_cast;

struct Scene {
    /* Styles */
    int windowPadding = 4;

    /* Sizes depends on ImGui::Style only */
    int sliderHeight = 0;

    /* Main window */
    int windowWidth = 0;
    int windowHeight = 0;
};

struct PlayState {
    bool paused = false;
    bool update = false;    // when paused or manual seek
    float progress = 0.f;   // [0; 100]
    int64_t framePts = 0;   // last seen frame pts 
    int64_t frameDur = 0;   // last seen frame duration
};


struct SlideDetector {
    steady_clock::time_point last;
    bool move = false;
    bool hold = false;
    bool paused = false;

    // Detects manual seeking: when user is changing the UI-slider by hands
    bool hasManualSeek(const steady_clock::time_point& now, PlayState& ps, bool changed, bool active) {
        if (!hold && active) {
            hold = true;
            paused = ps.paused;
            ps.paused = true;
        }
        else if (hold && !active) {
            hold = false;
            ps.paused = paused;
        }

        auto dt = duration_cast<milliseconds>(now - last).count();
        if (changed && !move) {
            move = true;
            last = now;
            //std::cout << "Slider: move start" << std::endl;
            return true;
        }
        if (changed && move && dt > 25) {
            last = now; 
            //std::cout << "Slider: keep moving " << std::endl;
            return true;
        }
        if (!changed && move && dt > 50) {
            move = false;
            last = now;
            //std::cout << "Slider: move stop" << std::endl;
            return true;
        }

        return false;
    }
};

struct PlayController {
    Render& render;
    VideoReader reader;
    StreamInfo info;
    FrameLoader loader;
    FrameQueue frameQ;
    PlayState ps;
    steady_clock::time_point lastUpdate;

    explicit PlayController(Render& render) : 
        render(render), 
        loader(reader) //todo: what? mb need refactoring?
        { } 
    
    bool open(const char* fileName) {
        if (reader.open(fileName)) {
            info = reader.getStreamInfo();
            return true;
        }
        return false;
    }
    
    void start(const steady_clock::time_point& now) {
        loader.createFrames(10, info.width, info.height);
        loader.start();
        lastUpdate = now;
    }

    void stop() {
        frameQ.flush(loader);
        loader.stop();
    }
    
    void seekProgress(float progress) {
        auto pts = info.progressToPts(ps.progress);
        seekPts(pts);
    }

    void seekLeft() {
        if (ps.paused) {
            ps.update = true;
            frameQ.seekPrevFrame(loader);
            frameQ.print();
        }
        else {
            // minus 1 second
            auto oneSecond = info.microsToPts(1000000);
            auto pts = std::max(0LL, ps.framePts - oneSecond);
            seekPts(pts);
        }
    }

    void seekRight() {
        if (ps.paused) {
            ps.update = true;
            frameQ.seekNextFrame(loader);
            frameQ.print();
        }
        else {
            // plus 1 second
            auto oneSecond = info.microsToPts(1000000);
            auto pts = std::min(info.durationPts, ps.framePts + oneSecond);
            seekPts(pts);
        }
    }

    void seekPts(int64_t pts) {
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

    void togglePause() {
        setPause(!ps.paused);
    }

    void setPause(bool paused) {
        if (paused) {
            ps.paused = true;
            ps.update = false;
            frameQ.print();
        }
        else {
            ps.paused = false;
            ps.update = false;
            frameQ.play(loader);
        }
    }

    bool hasUpdate(const steady_clock::time_point& now) {
        
        frameQ.fillFrom(loader);
        
        if (ps.paused && ps.update) {
            const RGBFrame* frame = frameQ.curr();
            if (frame) {
                ps.update = false;
                ps.framePts = frame->pts;
                ps.frameDur = frame->duration;
                ps.progress = info.calcProgress(frame->pts);
                return true;
            }
        }

        if (!ps.paused) {
            auto durationMicros = duration_cast<microseconds>(now - lastUpdate).count();
            auto durationPts = info.microsToPts(durationMicros);

            if (durationPts > ps.frameDur || ps.update) {
                const RGBFrame* frame = frameQ.next();
                if (frame) {
                    auto deltaPts = durationPts - ps.frameDur;
                    if (deltaPts < frame->duration) {
                        lastUpdate = now - microseconds(info.ptsToMicros(deltaPts));
                    } else {
                        lastUpdate = now;
                    }

                    ps.update = false;
                    ps.framePts = frame->pts;
                    ps.frameDur = frame->duration;
                    ps.progress = info.calcProgress(frame->pts);
                    return true;
                }
            }
        }

        return false;

    }

    const RGBFrame* currentFrame() {
        return frameQ.curr();
    }
};


Scene scene;
Render render;
PlayController player(render);


static void reshape(int w, int h) {
    
    //cout << "reshape w=" << w << " h=" << h << endl;

    scene.windowWidth = w;
    scene.windowHeight = h;

    const int frameWidth = scene.windowWidth / 2;
    const int frameHeight = scene.windowHeight - scene.sliderHeight;

    {
        int left = 0;
        int top = 0;
        int right = left + frameWidth;
        int bottom = top + frameHeight;
        render.frames[0].leftTop = { left, top };
        render.frames[0].viewPort = { left, scene.windowHeight - bottom };
        render.frames[0].viewSize = { right - left, bottom - top };
        render.frames[0].cam.reshape(right - left, bottom - top);
    }
    
    {
        int left = frameWidth;
        int top = 0;
        int right = left + frameWidth;
        int bottom = top + frameHeight;
        render.frames[1].leftTop = { left, top };
        render.frames[1].viewPort = { left, scene.windowHeight - bottom };
        render.frames[1].viewSize = { right - left, bottom - top };
        render.frames[1].cam.reshape(right - left, bottom - top);
    }
}
static void reshapeWindow(GLFWwindow*, int w, int h) {
    reshape(w, h);
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
        player.togglePause();
    }
    else if (key.is(LEFT)) {
        player.seekLeft();
    }
    else if (key.is(RIGHT)) {
        player.seekRight();
    }
}
static void mouseCallback(ui::mouse::MouseEvent event) {
    using namespace ui;
    using namespace ui::mouse;

    if (event.is(Action::MOVE, Button::LEFT)) {
        auto delta = event.getDelta();
        render.move(delta.x, delta.y);
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
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
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
    /*io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;*/
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

    //Update scene sizes depends on styles
    const ImGuiStyle& style = ImGui::GetStyle();
    scene.sliderHeight = ImGui::GetFontSize() + style.FramePadding.y * 2 + style.WindowPadding.y * 2;


    io.Fonts->GetGlyphRangesCyrillic();
}
static void destroyImGui() {
    ImGui::PopStyleVar(2);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


/*
    Todo:
        open file dialog - select file
        draw points on video
        select between modes:
            1. move/scale video
            2. draw on video
            3. playing/steps (?)
*/
//#include <Windows.h>
//#include <filesystem>

int main(int argc, char* argv[]) {

    // setlocale(LC_ALL, "Russian");

    //{
    //    using std::cout;
    //    using std::endl;
    //    namespace fs = std::filesystem;
    //    auto path = fs::path("C:\\Users\\Konst\\Desktop");//fs::current_path();
    //    std::cout << "exists() = " << fs::exists(path) << "\n"
    //        //<< "root_name() = " << path.root_name() << "\n"
    //        //<< "root_path() = " << path.root_path() << "\n"
    //        << "relative_path() = " << path.relative_path() << "\n"
    //        << "parent_path() = " << path.parent_path() << "\n"
    //        << "filename() = " << path.filename() << "\n"
    //        << "stem() = " << path.stem() << "\n"
    //        << "extension() = " << path.extension() << "\n"
    //        << "isDirectory = "<< fs::is_directory(path) << "\n";
    //    setlocale(LC_ALL, "Russian");
    //    SetConsoleOutputCP(1251);
    //    SetConsoleCP(1251);
    //    for (const auto& dir : fs::directory_iterator(path)) {
    //        const auto& localPath = dir.path();
    //        auto fileName = localPath.filename().wstring();
    //        std::wcout << fileName << endl;
    //    }
    //    return 0;
    //}
    
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
    reshape(windowWidth, windowHeight);


    const char* fileName = "C:/Users/Konst/Desktop/IMG_3504.MOV";
    if (!player.open(fileName)) {
        std::cout << "File open - error" << std::endl;
        return -1;
    }

    auto now = steady_clock::now();
    player.start(now);
    render.createFrame(0, player.info.width, player.info.height);
    render.createFrame(1, player.info.width, player.info.height);
  
    FpsCounter fps;
    SlideDetector slider;

    while (!glfwWindowShouldClose(window)) {

        auto now = steady_clock::now();
        if (player.hasUpdate(now)) {
            const RGBFrame* rgb = player.currentFrame();
            render.updateFrame(0, rgb->width, rgb->height, rgb->pixels);
            render.updateFrame(1, rgb->width, rgb->height, rgb->pixels);
            fps.print();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto* viewPort = ImGui::GetMainViewport();
        const auto& workSize = viewPort->WorkSize;

        {
            ImGui::SetNextWindowPos(ImVec2(0, workSize.y - scene.sliderHeight), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(workSize.x, scene.sliderHeight), ImGuiCond_Always);
            ImGui::Begin("SlidersWindow", nullptr,
                ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoNavInputs |
                ImGuiWindowFlags_NoNavFocus);

            const auto& style = ImGui::GetStyle();
            float itemWidth = 0.5 * (workSize.x - 3 * style.WindowPadding.x);
            ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;
            ImGui::PushItemWidth(itemWidth);
            {
                // todo: use static progress here instead of ps.progress?
                // update ps.progress inside the slider.update(...)?
                ImGui::PushID(0);
                PlayState& ps = player.ps;
                bool changed = ImGui::SliderFloat("", &ps.progress, 0.0f, 100.0f, "", flags);
                bool active = ImGui::IsItemActive();
                bool hasSeek = slider.hasManualSeek(now, ps, changed, active);
                if (hasSeek) {
                    player.seekProgress(ps.progress);
                }
                ImGui::PopID();
            }
            {
                ImGui::PushID(1);
                ImGui::SameLine(0.f, style.WindowPadding.x);
                static float progress = 0.f;
                bool changed = ImGui::SliderFloat("", &progress, 0.0f, 100.0f, "", flags);
                bool active = ImGui::IsItemActive();
                //todo: update another slider for another video stream
                //slider.update(changed, active, ps.progress);
                ImGui::PopID();
            }
            ImGui::PopItemWidth();
            ImGui::End();
        }
        
        {
            static bool opened = true;
            ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_FirstUseEver);
            ImGui::Begin("FolderWindow", &opened, ImGuiWindowFlags_None);
            ImGui::Text("MyObject1");
            ImGui::Text("MyObject2");
            ImGui::Text("MyObject3");
            ImGui::Text("MyObject4");
            ImGui::Text("MyObject5");
            ImGui::End();

           
        }        

        ImGui::DebugTextEncoding("Привет");

        render.draw();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glFlush();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

   
    player.stop();
    render.destroyFrames();
    render.destroyShaders();
    destroyImGui();
    glfwTerminate();
	return 0;
}
