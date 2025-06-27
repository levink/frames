#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <functional>
#include <filesystem>
#include "io/io.h"
#include "video/video.h"
#include "render.h"
#include "imgui.h"
#include "nfd.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace fs = std::filesystem;
using namespace video;
using std::cout;
using std::endl;
using std::list;
using std::function;
using std::string;
using std::chrono::steady_clock;


//menu commands
static bool openFileDialog(string& path) {
    static bool opened = false;

    if (opened) {
        return false;
    }

    opened = true;
    nfdchar_t* bytesUTF8 = nullptr;
    nfdresult_t result = NFD_OpenDialog(nullptr, nullptr, &bytesUTF8);
    if (result == NFD_OKAY) {
        path = string(bytesUTF8);
        NFD_Free(&bytesUTF8);
        opened = false;
        return true;
    }
    else if (result == NFD_CANCEL) { /*cout << "nfd cancel" << endl;*/ }
    else if (result == NFD_ERROR) { /*cout << "nfd error" << endl;*/ }

    opened = false;
    return false;
}
static bool openFolderDialog(string& path) {
    static bool opened = false;

    if (opened) {
        return false;
    }

    opened = true;
    nfdchar_t* bytesUTF8 = nullptr;
    nfdresult_t result = NFD_PickFolder(nullptr, &bytesUTF8);
    if (result == NFD_OKAY) {
        path = string(bytesUTF8);// fs::u8path(bytesUTF8);
        NFD_Free(&bytesUTF8);
        opened = false;
        return true;
    }
    else if (result == NFD_CANCEL) { /*cout << "nfd cancel" << endl;*/ }
    else if (result == NFD_ERROR) { /*cout << "nfd error" << endl;*/ }

    opened = false;
    return false;
}

//global commands
namespace cmd {
    static void playFile(Player* player, Frame* frame, const string& path) {
        if (player->open(path.c_str())) {
            const auto& info = player->info;
            frame->create(info.width, info.height);
            cout << "File open - ok: " << path << endl;
        }
        else {
            std::cout << "File open - error" << std::endl;
        }
    }
    static void openFile(Player* player, Frame* frame) {
        string path;
        if (openFileDialog(path)) {
            playFile(player, frame, path);
        }
    }
    static void togglePause(Player* player) {
        bool newValue = !(player->ps.paused);
        player->pause(newValue);
    }
    static void seekLeft(Player* player) {
        player->seekLeft();
    }
    static void seekRight(Player* player) {
        player->seekRight();
    }
}


Render render;
Player player;

int count = 0;
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
        frame.cam.zoom(io.MouseWheel * 0.1f);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        cout << "local x = " << mx << " local y=" << my << std::endl;
    }
}
static void keyCallback(GLFWwindow* window, int keyCode, int scanCode, int action, int mods) {
    using namespace io;
    using namespace io::keyboard;

    count++;
    cout << keyCode << " " << action << " " << mods << endl;

  /*  if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }*/

    auto key = KeyEvent(keyCode, action, mods);
    //auto prev = io::lastKeyPressEvent();
    if (key.is(ESC)) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (key.is(R)) {
        std::cout << "Reload shaders" << std::endl;
        render.reloadShaders();
    }
    else if (key.is(SPACE)) {
        cmd::togglePause(&player);
    }
    else if (key.is(LEFT)) {
        cmd::seekLeft(&player);
    }
    else if (key.is(RIGHT)) {
        cmd::seekRight(&player);
    }
    else if (key.is(KeyMod::CTRL, O)) {
       /* if (prev.is(KeyMod::CTRL, K)) {
            
        }
        else {
            cmd::openFile(&player, &render.frames[0]);
        }*/
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
static string toUTF8(const fs::path& path) {
    auto utf8 = path.u8string();
    return std::move(string(utf8.begin(), utf8.end()));
}


namespace ui {

    namespace dragDrop {
        constexpr static const char* PATH_TAG = "DRAG_DROP_PATH";
        static const void* packString(const string& value) {
            return &value;
        }
        static const string* unpackString(void* data) {
            return *(string**)(data);   // Take care =)
        }
    }

    static void newFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    static void render() {
        //ImGui::ShowDemoWindow();
        //ImGui::DebugTextEncoding("Привет");
        //ImGui::ShowMetricsWindow();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    struct Slider {
        float progress = 0;
        bool hold = false;
    private:
        float lastProgress = 0;
        time_point lastUpdate;
    public:
        bool update(bool active) {
            using std::chrono::milliseconds;

            if (!hold && active) {
                hold = true;
                updateValue(steady_clock::now());
                return true;
            }
            if (hold && !active) {
                hold = false;
                updateValue(steady_clock::now());
                return true;
            }
            if (hold) {
                auto now = steady_clock::now();
                auto delta = duration_cast<milliseconds>(now - lastUpdate).count();
                if (delta > 250) {
                    return updateValue(now);
                }
            }
            return false;
        }
    private:
        bool updateValue(const steady_clock::time_point& time) {
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

    struct MainMenuBar {
        function<void()> quit;
        function<void()> openFile;
        function<void()> openFolder;
        void show() {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open File", "Ctrl+O")) { openFile(); }
                    if (ImGui::MenuItem("Open Folder", "Ctrl+K Ctrl+O")) { openFolder(); }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Quit", "Alt+F4")) { quit(); }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
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
    };

    struct FrameWindow {
        const char* windowName;
        bool windowOpened = true;
        Viewport viewPort;
        Slider slider;
        function<void(int, int)> mouseFn;
        function<void(float, bool)> slideFn;
        function<void(const Viewport&)> reshapeFn;
        function<void(const string&)> acceptDropFn;

        explicit FrameWindow(const char* name) : windowName(name) { }
        void setProgress(float progress) {
            slider.progress = progress;
        }
        void show() {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            //ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
            //ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);

            constexpr int padding = 8;
            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBackground;
            ImGui::Begin(windowName, &windowOpened, flags);

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
                if (mouseFn && ImGui::IsItemHovered()) {
                    ImGuiIO& io = ImGui::GetIO();
                    auto localX = io.MousePos.x - cursor.x;
                    auto localY = io.MousePos.y - cursor.y;
                    mouseFn(localX, localY);
                }

                if (acceptDropFn && ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDrop::PATH_TAG)) {
                        auto value = dragDrop::unpackString(payload->Data);
                        acceptDropFn(*value);
                    }
                    ImGui::EndDragDropTarget();
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
                    slideFn(slider.progress, slider.hold);
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
    private:
        struct Node {
            bool folder = false;
            bool loaded = false;
            bool opened = false;
            fs::path path;
            string pathUTF8;
            string nameUTF8;
            list<Node> childreen;
        };

        const char* windowName = nullptr;
        bool windowOpened = true;
        Node root;
    
    public:
        explicit FolderWindow(const char* name) : windowName(name) {}
        void open(const string& pathUTF8) {
            fs::path path = fs::u8path(pathUTF8);
            if (fs::is_directory(path)) {
                root.folder = true;
                root.loaded = true;
                root.opened = true;
                root.path = path;
                root.pathUTF8 = toUTF8(path);
                root.nameUTF8 = toUTF8(path.filename());
                loadChildreen(path, root.childreen);
            }
            else {
                root.folder = false;
                root.loaded = false;
                root.opened = false;
                root.path = path;
                root.pathUTF8 = toUTF8(path);
                root.nameUTF8 = toUTF8(path.filename());
            }
        }
        void show() {
            if (windowOpened) {
                ImGui::Begin(windowName, &windowOpened, 
                    ImGuiWindowFlags_NoCollapse | 
                    ImGuiWindowFlags_NoTitleBar | 
                    ImGuiWindowFlags_HorizontalScrollbar);
                showChildreen(root);

                ImGui::End();
            }
        }
        void refresh() {
            /*
                Todo: implement this
                Need check correctness of all filenames & folders
                because structure might be changed from outside
            */
        }

    private:
        void loadChildreen(const fs::path& path, list<Node>& result) {
            result.clear();
            for (const auto& child : fs::directory_iterator(path)) {
                auto& childPath = child.path();

                Node node;
                node.folder = fs::is_directory(childPath);
                node.loaded = false;
                node.opened = false;
                node.path = childPath;
                node.pathUTF8 = toUTF8(childPath);
                node.nameUTF8 = toUTF8(childPath.filename());
                result.push_back(node);
            }

            auto foldersOnTop = [](const Node& left, const Node& right) {
                return left.folder > right.folder;
            };
            std::stable_sort(result.begin(), result.end(), foldersOnTop);
        }
        void showChildreen(Node& node) {
            ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_DrawLinesNone;
            if (node.opened) {
                base_flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }
            
            node.opened = false;
            if (ImGui::TreeNodeEx(node.nameUTF8.c_str(), base_flags)) {
                node.opened = true;
                if (!node.loaded) {
                    node.loaded = true;
                    loadChildreen(node.path, node.childreen);
                }
                for (auto& child : node.childreen) {
                    if (child.folder) {
                        showChildreen(child);
                    } else {
                        showFile(child);
                    }
                }
                ImGui::TreePop();
            }
        }
        void showFile(Node& node) {
            ImGui::Selectable(node.nameUTF8.c_str());
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                auto data = dragDrop::packString(node.pathUTF8);
                ImGui::SetDragDropPayload(dragDrop::PATH_TAG, &data, sizeof(data), ImGuiCond_Once);
                ImGui::Text(node.nameUTF8.c_str());
                ImGui::EndDragDropSource();
            }
        }
    };
}
/*
    Todo:
        keyboard cmd-s
        draw (points, lines, etc...) --> show demo after this
        two frames
        play buttons
        remember opened folder
        select mode for frame(?):
            1. move/scale video
            2. draw on video
            3. playing/steps (?)

        react on opening non video files (bad format error?)
        move nfd/include to ./include dir? + move nfd to separate lib?
        perfect would be render frames to the textures (RTT)
        and use them all in the imgui side
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
    render.frames[0].create(player.info.width, player.info.height);


    ui::MainMenuBar mainMenuBar;
    ui::FrameWindow frameWindow("frameWindow");
    ui::FolderWindow folderWindow("folderWindow");

    mainMenuBar.openFile = []() {
        string path;
        if (openFileDialog(path)) {
            cmd::playFile(&player, &render.frames[0], path);
        }
    };
    mainMenuBar.openFolder = [&folderWindow]() {       
        string path;
        if (openFolderDialog(path)) {
            folderWindow.open(path);
        }
    };
    mainMenuBar.quit = [&window]() {
        glfwSetWindowShouldClose(window, GL_TRUE);
    };
    frameWindow.mouseFn = [](int mx, int my) {
        mouseCallback(render.frames[0], mx, my);
    };
    frameWindow.slideFn = [](float progress, bool hold) {
        player.seekProgress(progress, hold);
    };
    frameWindow.reshapeFn = [](const ui::Viewport& vp) {
        const auto [left, top] = vp.cursor;
        const auto [width, height] = vp.region;
        render.frames[0].reshape(left, top, width, height, vp.screen.y);
    };
    frameWindow.acceptDropFn = [](const string& path) {
        cmd::playFile(&player, &render.frames[0], path);
    };
    
    auto defaultPath = toUTF8(fs::current_path());
    folderWindow.open(defaultPath);

    while (!glfwWindowShouldClose(window)) {

        auto now = steady_clock::now();
        if (player.hasUpdate(now)) { 
            const RGBFrame* rgb = player.currentFrame();
            render.frames[0].update(rgb->width, rgb->height, rgb->pixels);
            frameWindow.setProgress(player.ps.progress);
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        render.draw(); 

        ui::newFrame();
        mainMenuBar.show();
        folderWindow.show();
        frameWindow.show();
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
