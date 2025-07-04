#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <functional>
#include "io/io.h"
#include "util/fs.h"
#include "video/video.h"
#include "render.h"
#include "resources.h"
#include "imgui.h"
#include "nfd.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

using namespace video;
using std::cout;
using std::endl;
using std::list;
using std::function;
using std::string;
using std::chrono::steady_clock;

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
        ImVec2 region;
        bool update(const ImVec2& value) {
            bool changed = 
                region.x != value.x ||
                region.y != value.y;
            region = value;
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
                    if (ImGui::MenuItem("Open Folder", "Ctrl+K, O")) { openFolder(); }
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
        string id;
        char name[128];
        bool opened;
        bool hovered;
        Viewport viewPort;
        Slider slider;
        function<void(bool)> hoverFn;
        function<void(int, int)> mouseFn;
        function<void(float, bool)> slideFn;
        function<void(const Viewport&)> reshapeFn;
        function<void(const string&)> acceptDropFn;

        explicit FrameWindow(const char* name, const char* id) : 
            id(id), 
            opened(true), 
            hovered(false) {
            setName(name);
        }
        void setName(const char* label) {
            memset(name, 0, sizeof(name));
            snprintf(name, 128, "%s##%s", label, id.c_str());
        }
        void setProgress(float progress) {
            slider.progress = progress;
        }
        void show(const ImTextureRef& texture) {
          
            //ImGui::SetNextWindowPos(position, ImGuiCond_FirstUseEver);
            //ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
            //ImGui::SetNextWindowBgAlpha(0.1f);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); //no padding
            ImGui::Begin(name, &opened, ImGuiWindowFlags_NoCollapse);

            auto cursor = ImGui::GetCursorScreenPos();
            auto region = ImGui::GetContentRegionAvail();

            // invisible button
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));   
                bool changed = viewPort.update(region);
                if (changed && reshapeFn) {
                    reshapeFn(viewPort);
                }
                ImGui::InvisibleButton("full_size_area", region, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
                bool itemHovered = ImGui::IsItemHovered();
                if (hoverFn && hovered != itemHovered) {
                    hovered = itemHovered;
                    hoverFn(hovered);
                }
                if (mouseFn && hovered) {
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
                ImGui::PopStyleVar();
            }

            // video texture (RTT)
            ImTextureID textureId = texture.GetTexID();
            if (textureId != ImTextureID_Invalid) {
                ImGui::SetCursorScreenPos(cursor);
                ImGui::Image(textureId, region, ImVec2(0, 1), ImVec2(1, 0));
            }
            
            // slider
            {  
                constexpr int padding = 8;
                auto cursor = ImGui::GetCursorScreenPos();
                cursor.y -= ImGui::GetFrameHeightWithSpacing() + padding * 2;
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
        void openFolder(const string& pathUTF8) {
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
        void openFile(const string& pathUTF8) {
            //todo: implement this
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
                    }
                    else {
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

    struct FrameController {
        Player& player;
        FrameRender& frameRender;
        FrameWindow& frameWindow;
        ImTextureRef frameTexture;
        bool active = true;
        void linkChildreen();
        void update(const time_point& now);
        void openFile(const string& path);
        void togglePause();
        void seekLeft();
        void seekRight();
        void clearDrawn();
    };
}

Render render;
Player player0;
Player player1;
ui::MainMenuBar mainMenuBar;
ui::FolderWindow folderWindow("##folderWindow");
ui::FrameWindow frameWindow_0("Frame 0", "##frameWindow_01");
ui::FrameWindow frameWindow_1("Frame 1", "##frameWindow_1");
ui::FrameController fc[2] = {
    { player0, render.frames[0], frameWindow_0 },
    { player1, render.frames[1], frameWindow_1 }
};


static bool showNativeFileDialog(string& path, bool folder) {
    static bool opened = false;

    if (opened) {
        return false;
    }

    opened = true;
    nfdchar_t* bytesUTF8 = nullptr;
    nfdresult_t result = folder ? 
        NFD_PickFolder(nullptr, &bytesUTF8) :
        NFD_OpenDialog(nullptr, nullptr, &bytesUTF8);
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
static void openFileCommand() {
    string path;
    if (showNativeFileDialog(path, false)) {
        folderWindow.openFile(path);
    }
}
static void openFolderCommand() {
    string path;
    if (showNativeFileDialog(path, true)) {
        folderWindow.openFolder(path);
    }
}
static void mouseCallback(FrameRender& frame, int mx, int my) {
  
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        int dx = io.MouseDelta.x;
        int dy = io.MouseDelta.y;
        if (dx || dy) {
            frame.moveCam(dx, dy);
        }
    }
    if (io.MouseWheel) {
        bool shift = ImGui::IsKeyDown(ImGuiMod_Shift);
        float value = shift ? (io.MouseWheel * 0.25f) : io.MouseWheel;
        frame.zoomCam(value);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        frame.setBrush({ 1.f, 0.f, 0.f }, 20.f);
        frame.drawStart(mx, my);
    } 
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        frame.drawStop();
    }
    else if (io.MouseDelta.x || io.MouseDelta.y) {
        bool pressed = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        frame.drawNext(mx, my, pressed);
    }    
}
static void keyCallback(GLFWwindow* window, int keyCode, int scanCode, int action, int mods) {
    using namespace io;
    using namespace io::keyboard;

    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }

    const KeyEvent& key = keyboard::create(keyCode, action, mods);
    if (key.is(ESC)) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (key.is(R)) {
        std::cout << "Reload shaders" << std::endl;
        render.reloadShaders();
    }
    else if (key.is(SPACE)) {
        fc[0].togglePause();
        fc[1].togglePause();
    }
    else if (key.is(LEFT)) {
        fc[0].seekLeft(); //todo: longseek
        fc[1].seekLeft();
    }
    else if (key.is(RIGHT)) {
        fc[0].seekRight();//todo: longseek
        fc[1].seekRight();
    }
    else if (key.is(X)) {
        fc[0].clearDrawn();
        fc[1].clearDrawn();
    }
    else if (key.is(Mod::CONTROL, K, O)) {
        openFolderCommand();
    }
    else if (key.is(Mod::CONTROL, O)) {
        openFileCommand();
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
    io.Fonts->AddFontFromFileTTF(resources::font, 14);
    io.Fonts->Build();   
}
static void destroyImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


void ui::FrameController::linkChildreen() {
    frameWindow.hoverFn = [this](bool hovered) {
        if (hovered) {
            frameRender.showCursor(true);
        }
        else {
            frameRender.showCursor(false);
            frameRender.drawStop();
        }
    };
    frameWindow.mouseFn = [this](int mx, int my) {
        mouseCallback(frameRender, mx, my);
    };
    frameWindow.slideFn = [this](float progress, bool hold) {
        player.seekProgress(progress, hold);
    };
    frameWindow.reshapeFn = [this](const ui::Viewport& vp) {
        const auto [width, height] = vp.region;
        frameRender.reshape(width, height);
    };
    frameWindow.acceptDropFn = [this](const string& path) {
        this->openFile(path);
    };
    frameTexture = ImTextureRef(frameRender.fb.tid);
}
void ui::FrameController::update(const time_point& now) {
    //todo: check player is opened
    if (player.hasUpdate(now)) {
        const RGBFrame* rgb = player.currentFrame();
        frameRender.updateTexture(rgb->width, rgb->height, rgb->pixels);
        frameWindow.setProgress(player.ps.progress);
    }
}
void ui::FrameController::openFile(const string& path) {
    if (player.start(path.c_str())) {
        const auto fileName = fs::path(path).filename().string();
        const auto& info = player.info;
        frameRender.clearDrawn();
        frameRender.createTexture(info.width, info.height);
        frameWindow.setName(fileName.c_str());
        cout << "File open - ok: " << path << endl;
    }
    else {
        std::cout << "File open - error" << std::endl;
    }
}
void ui::FrameController::togglePause() {
    if (active) {
        bool newValue = !(player.ps.paused);
        player.pause(newValue);
    }

}
void ui::FrameController::seekLeft() {
    if (active) {
        player.seekLeft();
    }
}
void ui::FrameController::seekRight() {
    if (active) {
        player.seekRight();
    }
}
void ui::FrameController::clearDrawn() {
    frameRender.clearDrawn();
}

/*
    Todo:
        remember opened folder
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
    render.createShaders();
    render.createFrameBuffers();
    initWindow(window);
    initImGui(window);
    
    mainMenuBar.openFile = []() {
        openFileCommand();
    };
    mainMenuBar.openFolder = []() {     
        openFolderCommand();
    };
    mainMenuBar.quit = [&window]() {
        glfwSetWindowShouldClose(window, GL_TRUE);
    };
    fc[0].linkChildreen();
    fc[1].linkChildreen();
    
    auto defaultPath = toUTF8(fs::current_path());
    folderWindow.openFolder(defaultPath);

    auto defaultFilePath = "C:/Users/Konst/Desktop/IMG_3504.MOV";
    fc[0].openFile(defaultFilePath);

    fc[0].frameRender.setBrush({ 1.f, 0.f, 0.f }, 20.f);
    fc[1].frameRender.setBrush({ 1.f, 0.f, 0.f }, 20.f);

    while (!glfwWindowShouldClose(window)) {

        auto now = steady_clock::now();
        fc[0].update(now);
        fc[1].update(now); 

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ui::newFrame();
        mainMenuBar.show();
        folderWindow.show();
        frameWindow_0.show(fc[0].frameTexture);
        frameWindow_1.show(fc[1].frameTexture);
        render.render();
        ui::render();

        glFlush();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    player0.stop();
    player1.stop();
    render.destroyFrames();
    render.destroyShaders();
    render.destroyFrameBuffers();
    destroyImGui();
    glfwTerminate();
	return 0;
}
