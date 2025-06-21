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

struct Settings {
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
        auto pts = info.progressToPts(progress);
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
        pause(!ps.paused);
    }

    void pause(bool paused) {
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

Render render;
Settings settings;
PlayController player(render);

static void reshape(int w, int h) {
    
    settings.windowWidth = w;
    settings.windowHeight = h;

    const int frameWidth = w / 2;
    const int frameHeight = h;

    {
        int left = 0;
        int top = 0;
        int right = left + frameWidth;
        int bottom = top + frameHeight;
        render.frames[0].leftTop = { left, top };
        render.frames[0].viewPort = { left, frameHeight - bottom };
        render.frames[0].viewSize = { right - left, bottom - top };
        render.frames[0].cam.reshape(right - left, bottom - top);
    }
    
    {
        int left = frameWidth;
        int top = 0;
        int right = left + frameWidth;
        int bottom = top + frameHeight;
        render.frames[1].leftTop = { left, top };
        render.frames[1].viewPort = { left, frameHeight - bottom };
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
        //render.move(delta.x, delta.y);
    }
    else if (event.is(Action::MOVE, Button::NO)) {
        auto cursor = event.getCursor();
        //render.select(cursor);
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

  /*  auto mx = static_cast<int>(x);
    auto my = static_cast<int>(y);
    auto event = ui::mouse::move(mx, my);
    mouseCallback(event);*/
}
static void mouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    //render.zoom(yoffset);
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

    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("../../data/fonts/calibri.ttf", 14);
    io.Fonts->Build();   
}
static void destroyImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

namespace ui {
    static void start() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    static void draw() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }   

    struct Slider {
        float progress = 0;
        bool changed = false;
    private:
        bool hold = false;
        float progressLast = 0;
        steady_clock::time_point lastUpdate;
    public:
        void update(bool active) {
            changed = false;

            if (!hold && active) {
                hold = true;
                changed = valueChanged(steady_clock::now());
                return;
            }
            if (hold && !active) {
                hold = false;
                changed = valueChanged(steady_clock::now());
                return;
            }
            if (hold) {
                auto now = steady_clock::now();
                auto delta = duration_cast<milliseconds>(now - lastUpdate).count();
                if (delta > 250) {
                    changed = valueChanged(now);
                }
            }
        }

    private:
        bool valueChanged(const steady_clock::time_point& time) {
            if (progressLast != progress) {
                progressLast = progress;
                lastUpdate = time;
                return true;
            }
            return false;
        }
    };

    struct Viewport {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
    };

    struct FrameView {
        const char* name;
        Viewport viewPort;
        Slider slider;
        
        explicit FrameView(const char* name) : name(name) { }
        void draw(const ImVec2& position, const ImVec2& size) {

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::SetNextWindowPos(position, ImGuiCond_Appearing);
            ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);

            constexpr int padding = 8;
            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoBackground;
            ImGui::Begin(name, nullptr, flags);

            ImGui::IsWindowCollapsed();

            //frame
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                ImGui::BeginChild("frame", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - padding * 2));
                ImVec2 view_p0 = ImGui::GetCursorScreenPos();
                ImVec2 view_sz = ImGui::GetContentRegionAvail();
                const int offsetFromWindowLeft = view_p0.x;
                const int offsetFromWindowBottom = ImGui::GetMainViewport()->WorkSize.y - (view_p0.y + view_sz.y);
                viewPort.x = offsetFromWindowLeft;
                viewPort.y = offsetFromWindowBottom;
                viewPort.w = view_sz.x;
                viewPort.h = view_sz.y;

                const ImVec2 size = ImGui::GetContentRegionAvail();
                ImGui::InvisibleButton("test", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
                /*const bool button_hovered = ImGui::IsItemHovered();
                const bool button_active = ImGui::IsItemActive();
                if (button_hovered) {
                    ImGuiIO& io = ImGui::GetIO();
                    auto screen = io.MousePos;
                    auto local = ImVec2(screen.x - view_p0.x, screen.y - view_p0.y);
                    //cout << "io x = " << local.x<< " y=" << local.y<< std::endl;
                }*/
                ImGui::EndChild();
                ImGui::PopStyleVar();
            }

            //slider
            {
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
                ImGui::BeginChild("slider", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##slider", &slider.progress, 0.0f, 100.0f, "", ImGuiSliderFlags_AlwaysClamp);
                slider.update(ImGui::IsItemActive());
                ImGui::PopItemWidth();
                ImGui::EndChild();
                ImGui::PopStyleVar();
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }
    };
    
}

/*
    Todo:
        bug:  player with init paused shows black screen

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
    player.pause(true); //todo: bug here. Shows black screen.
    render.createFrame(0, player.info.width, player.info.height);
    render.createFrame(1, player.info.width, player.info.height);
  
    FpsCounter fps;
    ui::FrameView frameView("frame_1");

    while (!glfwWindowShouldClose(window)) {

        auto now = steady_clock::now();

        if (player.hasUpdate(now)) {
            const RGBFrame* rgb = player.currentFrame();
            render.updateFrame(0, rgb->width, rgb->height, rgb->pixels);
            render.updateFrame(1, rgb->width, rgb->height, rgb->pixels);
            //player.frameQ.print();
            //fps.print();
        }

        ui::start();
        frameView.draw(ImVec2(50, 50), ImVec2(400, 500));
        if (frameView.slider.changed) {
            player.seekProgress(frameView.slider.progress);
        }
        const auto& vp = frameView.viewPort;
        render.frames[0].viewPort = { vp.x, vp.y };
        render.frames[0].viewSize = { vp.w, vp.h };


        //ImVec2 workSize = ImGui::GetMainViewport()->WorkSize;
        //ui::createFrameWindow("frame_2", ImVec2(500, 200), ImVec2(200, 200));
        
        //ImVec2 workSize = ImGui::GetMainViewport()->WorkSize;
        //const ImGuiStyle& style = ImGui::GetStyle();
        //{

        //    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        //    //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));

        //    int height = ImGui::GetFrameHeight() + style.WindowPadding.y * 2;

        //    ImGui::SetNextWindowPos(ImVec2(0, workSize.y - height), ImGuiCond_Always);
        //    ImGui::SetNextWindowSize(ImVec2(workSize.x, height), ImGuiCond_Always);
        //    ImGui::Begin("SlidersWindow", nullptr,
        //        //ImGuiWindowFlags_NoBackground |
        //        ImGuiWindowFlags_NoDecoration |
        //        ImGuiWindowFlags_NoNav
        //    );

        //    float itemWidth = 0.5 * (workSize.x - 3 * style.WindowPadding.x);
        //    ImGui::PushItemWidth(itemWidth);
        //    {
        //        // todo: use static progress here instead of ps.progress?
        //        // update ps.progress inside the slider.update(...)?
        //        ImGui::PushID(0);
        //        PlayState& ps = player.ps;
        //        bool changed = ImGui::SliderFloat("", &ps.progress, 0.0f, 100.0f, "", ImGuiSliderFlags_AlwaysClamp);
        //        bool active = ImGui::IsItemActive();
        //        bool hasSeek = slider.hasManualSeek(now, ps, changed, active);
        //        if (hasSeek) {
        //            player.seekProgress(ps.progress);
        //        }
        //        ImGui::PopID();
        //    }
        //    {
        //        ImGui::PushID(1);
        //        ImGui::SameLine(0.f, style.WindowPadding.x);
        //        static float progress = 0.f;
        //        bool changed = ImGui::SliderFloat("", &progress, 0.0f, 100.0f, "", ImGuiSliderFlags_AlwaysClamp);
        //        bool active = ImGui::IsItemActive();
        //        //todo: update another slider for another video stream
        //        //slider.update(changed, active, ps.progress);
        //        ImGui::PopID();
        //    }
        //    ImGui::PopItemWidth();
        //    ImGui::End();
        //    ImGui::PopStyleVar(1);
        //}



        //ImGui::ShowDemoWindow();
        //ImGui::DebugTextEncoding("Привет");
        //ImGui::ShowMetricsWindow();
        
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        render.draw();
        ui::draw();

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
