#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <functional>
#include <filesystem>
#include "ui/ui.h"
#include "util/fpscounter.h"
#include "video/video.h"
#include "render.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace fs = std::filesystem;
using std::cout;
using std::endl;
using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::function;
using std::list;
using std::string;

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
    
    void start() {
        loader.createFrames(10, info.width, info.height);
        loader.start();
        lastUpdate = steady_clock::now();
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
            ps.update = true;
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
PlayController player(render);

static void mouseCallback(Frame& frame, int mx, int my) {

    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        int dx = io.MouseDelta.x;
        int dy = io.MouseDelta.y;
        if (dx || dy) {
            frame.cam.move(dx, -dy);
        }
    }
    if (io.MouseWheel) {
        //cout << "scroll= " << io.MouseWheel << endl;
        frame.cam.zoom(io.MouseWheel * 0.1f);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        cout << "local x = " << mx << " local y=" << my << std::endl;
    }
}
static void keyCallback(GLFWwindow* window, int keyCode, int scanCode, int action, int mods) {
    using namespace ui::keyboard;
    
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) {
        return;
    }

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
    glfwSetKeyCallback(window, keyCallback);
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
    
    static void render() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }   

    static void showDebug() {
        ImGui::ShowDemoWindow();
        //ImGui::DebugTextEncoding("Привет");
        //ImGui::ShowMetricsWindow();
    }

    static void showMainMenuBar() {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
              /*  ShowExampleMenuFile();*/
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
                if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {} // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X")) {}
                if (ImGui::MenuItem("Copy", "CTRL+C")) {}
                if (ImGui::MenuItem("Paste", "CTRL+V")) {}
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
  
    struct Slider {
        float progress = 0;
    private:
        bool hold = false;
        float lastProgress = 0;
        steady_clock::time_point lastUpdate;
    public:
        bool update(bool active) {
            if (!hold && active) {
                hold = true;
                return valueChanged(steady_clock::now());
            }
            if (hold && !active) {
                hold = false;
                return valueChanged(steady_clock::now());
            }
            if (hold) {
                auto now = steady_clock::now();
                auto delta = duration_cast<milliseconds>(now - lastUpdate).count();
                if (delta > 250) {
                    return valueChanged(now);
                }
            }
            return false;
        }
    private:
        bool valueChanged(const steady_clock::time_point& time) {
            if (lastProgress != progress) {
                lastProgress = progress;
                lastUpdate = time;
                return true;
            }
            return false;
        }
    };

    struct Viewport {
        ImVec2 cursor;
        ImVec2 region;
        ImVec2 screen;
        bool update(const ImVec2& cursor, const ImVec2& region, const ImVec2& screen) {
            bool changed = 
                this->cursor.x != cursor.x || 
                this->cursor.y != cursor.y ||
                this->region.x != region.x || 
                this->region.y != region.y ||
                this->screen.x != screen.x ||
                this->screen.y != screen.y;
            if (changed) {
                this->cursor = cursor;
                this->region = region;
                this->screen = screen;
            }
            return changed;
        }
    };

    struct FrameWindow {
        const char* name;
        ImVec2 position;
        ImVec2 size;
        bool opened = true;
        Viewport viewPort;
        Slider slider;
        function<void(int, int)> mouseFn;
        function<void(float)> slideFn;
        function<void(const Viewport&)> reshapeFn;

        explicit FrameWindow(const char* name) : name(name) { }
        void setProgress(float progress) {
            slider.progress = progress;
        }
        void show() {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

            constexpr int padding = 8;
            ImGuiWindowFlags flags =
                //ImGuiWindowFlags_NoTitleBar |
                //ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBackground;
            ImGui::Begin(name, &opened, flags);

            //frame
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                ImGui::BeginChild("frame");
                auto cursor = ImGui::GetCursorScreenPos();
                auto region = ImGui::GetContentRegionAvail();
                auto screen = ImGui::GetMainViewport()->Size;
                bool changed = viewPort.update(cursor, region, screen);
                if (changed && reshapeFn) {
                    reshapeFn(viewPort);
                }

                ImGui::InvisibleButton("full_size_area", region, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
                if (ImGui::IsItemHovered() && mouseFn) {
                    ImGuiIO& io = ImGui::GetIO();
                    auto localX = io.MousePos.x - cursor.x;
                    auto localY = io.MousePos.y - cursor.y;
                    mouseFn(localX, localY);
                }

                ImGui::EndChild();
                ImGui::PopStyleVar();
            }

            //slider
            {
                auto cursor = ImGui::GetCursorScreenPos();
                cursor.y -= ImGui::GetFrameHeight() + padding * 2;
                ImGui::SetCursorScreenPos(cursor);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
                ImGui::BeginChild("slider", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##slider", &slider.progress, 0.0f, 100.0f, "", ImGuiSliderFlags_AlwaysClamp);
                bool changedByUser = slider.update(ImGui::IsItemActive());
                if (changedByUser && slideFn) {
                    slideFn(slider.progress);
                }
                ImGui::PopItemWidth();
                ImGui::EndChild();
                ImGui::PopStyleVar();
            }

            ImGui::End();
            ImGui::PopStyleVar();
        }
    };

    struct FolderWindow {

        struct Node {
            bool folder = false;
            bool loaded = false;
            bool opened = false;
            fs::path path;
            string name;
            list<Node> childreen;
        };

        const char* name;
        bool opened = true;
        Node root;

        explicit FolderWindow(const char* name) : name(name) {}
        void open(const fs::path& path) {
            if (fs::is_directory(path)) {
                root.folder = true;
                root.loaded = true;
                root.opened = true;
                root.path = path;
                root.name = toUTF8(path);
                loadFolder(path, root.childreen);
            }
            else {
                root.folder = false;
                root.loaded = false;
                root.opened = false;
                root.path = path;
                root.name = toUTF8(path.filename());
            }
        }
        void show() {
            if (opened) {
                ImGui::Begin(name, &opened);
                showFolder(root);
                ImGui::End();
            }
        }
        
        //Todo: implement this
        void refresh() {
            /*
                Need check correctness of all filenames & folders
                because structure might be changed from outside
            */
        }

    private:
        void loadFolder(const fs::path& path, list<Node>& result) {
            for (const auto& child : fs::directory_iterator(path)) {
                auto& childPath = child.path();

                Node node;
                node.folder = fs::is_directory(childPath);
                node.loaded = false;
                node.opened = false;
                node.path = childPath;
                node.name = toUTF8(childPath.filename());
                result.push_back(node);
            }

            auto foldersOnTop = [](const Node& left, const Node& right) {
                return left.folder > right.folder;
            };
            std::stable_sort(result.begin(), result.end(), foldersOnTop);
        }
        void showFolder(Node& node) {
            ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_DrawLinesNone;
            if (node.opened) {
                base_flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }
            
            node.opened = false;
            if (ImGui::TreeNodeEx(node.name.c_str(), base_flags)) {
                node.opened = true;
                if (!node.loaded) {
                    node.loaded = true;
                    loadFolder(node.path, node.childreen);
                }
                for (auto& child : node.childreen) {
                    if (child.folder) {
                        showFolder(child);
                    } else {
                        showFile(child);
                    }
                }
                ImGui::TreePop();
            }
        }
        void showFile(Node& node) {
            ImGui::Selectable(node.name.c_str());
        }
        string toUTF8(const fs::path& path) {
            auto utf8 = path.u8string();
            return std::move(string(utf8.begin(), utf8.end()));
        }
    };
}
/*
    Todo:
        open file dialog - select file
        drag & drop video
        draw points on video
        select between modes:
            1. move/scale video
            2. draw on video
            3. playing/steps (?)
*/

int main(int argc, char* argv[]) {
    if (!glfwInit()) {
        std::cout << "glfwInit error" << std::endl;
        return -1;
    }

    int windowWidth = 1500;
    int windowHeight = 900;
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

    const char* fileName = "C:/Users/Konst/Desktop/IMG_3504.MOV";
    if (!player.open(fileName)) {
        std::cout << "File open - error" << std::endl;
        return -1;
    }

    player.start();
    player.pause(true);
    render.createFrame(0, player.info.width, player.info.height);
    //render.createFrame(1, player.info.width, player.info.height);
  
    FpsCounter fps;
    ui::FrameWindow frameWindow("frameWindow");
    frameWindow.position = ImVec2(50, 50);
    frameWindow.size = ImVec2(400, 500);
    frameWindow.mouseFn = [](int mx, int my) {
        mouseCallback(render.frames[0], mx, my);
    };
    frameWindow.slideFn = [](float progress) {
        player.seekProgress(progress);
    };
    frameWindow.reshapeFn = [](const ui::Viewport& vp) {
        const auto [left, top] = vp.cursor;
        const auto [width, height] = vp.region;
        render.frames[0].reshape(left, top, width, height, vp.screen.y);
    };

    ui::FolderWindow folderWindow("folderWindow");
    folderWindow.open(fs::path("C:\\Users\\Konst\\Desktop"));

    while (!glfwWindowShouldClose(window)) {

        auto now = steady_clock::now();
        if (player.hasUpdate(now)) { 
            //todo: pause player when user holds slider
            const RGBFrame* rgb = player.currentFrame();
            render.updateFrame(0, rgb->width, rgb->height, rgb->pixels);
            frameWindow.setProgress(player.ps.progress);
            //player.frameQ.print();
            //fps.print();
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        //todo: 
        // perfect would be render frames to the textures (RTT)
        // and use them all in the imgui side
        render.draw(); 

        ui::start();
        ui::showMainMenuBar();
        folderWindow.show();
        frameWindow.show();
        ui::showDebug();
        ui::render();

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
