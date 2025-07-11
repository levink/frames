#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <functional>
#include "io/io.h"
#include "util/filedialog.h"
#include "util/fs.h"
#include "video/video.h"
#include "render.h"
#include "resources.h"
#include "workstate.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

using namespace video;
using std::cout;
using std::endl;
using std::list;
using std::function;
using std::string;
using std::chrono::steady_clock;

namespace cmd {
    static void openFile();
    static void openFolder();
    static void quitProgram();
}

namespace ui {

    namespace dragDrop {
        constexpr static const char* PATH_TAG = "DRAG_DROP_PATH";
        static const void* packPath(const fs::path* pointer) {
            return pointer;
        }
        static const fs::path* unpackPath(void* data) {
            return *(fs::path**)(data);   // Take care =)
        }
    }

    struct MainWindow {
    private:
        GLFWwindow* win = nullptr;
        int width = 1000;
        int height = 800;
    public:
        GLFWwindow* create() {
            win = glfwCreateWindow(width, height, resources::programName, nullptr, nullptr);
            return win;
        }
        void reshape(int w, int h) {
            width = w;
            height = h;
        }
        void setSize(int w, int h) {
            if (win) {
                glfwSetWindowSize(win, w, h);
            }
        }
        void close() const {
            if (win) {
                glfwSetWindowShouldClose(win, GL_TRUE);
            }
        }
        void saveState(WorkState& ws) const {
            auto& state = ws.main;
            state.width = width;
            state.height = height;
        }
        void restoreState(const WorkState& ws) {
            auto& state = ws.main;
            width = state.width;
            height = state.height;
        }
    };

    //todo: move inside frame window?
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

    struct FrameWindow {
        char id[20];
        char name[128];
        bool opened;
        bool frameHovered;
        bool slideHovered;
        ImTextureID textureId;
        ImVec2 size;
        Slider slider;
        function<void(bool)> hoverFrameFn;
        function<void(bool)> hoverSlideFn;
        function<void(int, int)> mouseFn;
        function<void(float, bool)> slideFn;
        function<void(const ImVec2&)> reshapeFn;
        function<void(const string&)> acceptDropFn;
        function<void(void)> closeFn;

        explicit FrameWindow(const char* name, const char* id) :
            id({ 0 }),
            opened(false), 
            frameHovered(false),
            slideHovered(false),
            textureId(ImTextureID_Invalid) {
            strncpy(this->id, id, sizeof(this->id));
            setName(name);
        }
        void setName(const char* label) {
            
            /*
                The 'name' buffer should contains "{label}{id}" where
                    {id} is ANSI string
                    {label} is UTF8 string

                We are control id, but not a label and its size. 
                So we should trim the label if it is too long. 
                And copy to the 'name' buffer only trimmed part of the label.
               
                Todo:
                    calc trim size for label more accurate cause it is utf8 string
            */

            constexpr size_t bufSize = sizeof(name) - 1; //reserve last byte for '\0'
            const size_t idSize = strlen(id);
            const size_t available = bufSize >= idSize ? bufSize - idSize : 0;
            const size_t labelSize = strlen(label);
            const size_t trimSize = std::min(available, labelSize);

            memset(name, 0, sizeof(name));
            snprintf(name, trimSize + 1, "%s", label);
            snprintf(name + trimSize, idSize + 1, "%s", id);
        }
        void setProgress(float progress) {
            slider.progress = progress;
        }
        void setTextureID(const ImTextureID& value) {
            textureId = value;
            opened = (textureId != ImTextureID_Invalid);
        }
        void draw() {
            bool hasVideo = textureId != ImTextureID_Invalid;
            bool openedPrevFrame = opened;

            ImGui::SetNextWindowBgAlpha(0.1f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

            auto flags = 
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoMove;
            if (!hasVideo) {
                flags |= ImGuiWindowFlags_NoTitleBar;
            }
            ImGui::Begin(name, &opened, flags);

            auto cursor = ImGui::GetCursorScreenPos();
            auto region = ImGui::GetContentRegionAvail();
            bool changed = updateSize(region);
            if (changed && reshapeFn) {
                reshapeFn(region);
            }

            // video texture (RTT)
            if (hasVideo) {
                ImGui::Image(textureId, ImVec2(region.x, region.y), ImVec2(0, 1), ImVec2(1, 0));
            }

            // invisible button
            {
                ImGui::SetCursorScreenPos(ImVec2(cursor.x, cursor.y));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4)); 
                ImGui::BeginChild("invisible_btn_parent", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
                ImGui::InvisibleButton("invisible_btn", ImGui::GetContentRegionAvail(), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
                
                bool hovered = ImGui::IsItemHovered();
                if (hoverFrameFn && frameHovered != hovered) {
                    frameHovered = hovered;
                    hoverFrameFn(hovered);
                }

                if (mouseFn && frameHovered) {
                    ImGuiIO& io = ImGui::GetIO();
                    auto localX = io.MousePos.x - cursor.x;
                    auto localY = io.MousePos.y - cursor.y;
                    mouseFn(localX, localY);
                }
                if (acceptDropFn && ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(dragDrop::PATH_TAG)) {
                        auto path = dragDrop::unpackPath(payload->Data);
                        auto str = toUTF8(*path);
                        acceptDropFn(str);
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::EndChild();
                ImGui::PopStyleVar();
            }

            // slider
            if (hasVideo) {
                constexpr int padding = 8;
                auto cursor = ImGui::GetCursorScreenPos();
                cursor.y -= ImGui::GetFrameHeightWithSpacing() + padding * 2 + 4;
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

                bool hovered = ImGui::IsItemHovered();                
                if (hoverSlideFn && slideHovered != hovered) {
                    slideHovered = hovered;
                    hoverSlideFn(hovered);
                }

                ImGui::PopStyleVar();
            }

            ImGui::End();
            ImGui::PopStyleVar();

            if (!opened && openedPrevFrame && closeFn) {
                closeFn();
            }
        }
        bool updateSize(const ImVec2& newSize) {
            bool changed =
                size.x != newSize.x ||
                size.y != newSize.y;
            size = newSize;
            return changed;
        }
    };

    struct FileTreeWindow {
    private:

        struct FileItem {
            fs::path path;
            string name;
        };

        struct TreeNode {
            bool folder = false;
            bool loaded = false;
            bool expanded = false;
            fs::path path;
            string name;
            list<TreeNode> childreen;
        };

        char searchBuffer[32] = { 0 };
        list<FileItem> files;
        TreeNode folder;
        bool needCloseFolder = false;

    public:
        void openFolder(const string& pathUTF8) {
            fs::path path = fs::u8path(pathUTF8);
            bool exists = fs::exists(path);
            if (!exists) {
                return;
            }

            if (fs::is_directory(path)) {
                folder.folder = true;
                folder.loaded = true;
                folder.expanded = true;
                folder.path = path;
                folder.name = toUTF8(path.filename());
                loadChildreen(folder);
            }
            else {
                folder.folder = false;
                folder.loaded = false;
                folder.expanded = false;
                folder.path = path;
                folder.name = toUTF8(path.filename());
            }
        }
        void closeFolder() {
            folder.folder = false;
            folder.loaded = false;
            folder.expanded = false;
            folder.path.clear();
            folder.name.clear();
        }
        void openFile(const string& pathUTF8) {
            fs::path path = fs::u8path(pathUTF8);
            bool exists = fs::exists(path);
            if (!exists) {
                return;
            }

            string name = toUTF8(path.filename());
            files.emplace_front(FileItem{
                std::move(path),
                std::move(name)
            });
        }
        void saveState(WorkState& ws) const {
            auto& state = ws.fileTree;
            state.folder = toUTF8(folder.path);
            state.files.reserve(files.size());
            for (auto& item : files) {
                state.files.push_back(toUTF8(item.path));
            }
        }
        void restoreState(const WorkState& ws) {
            auto& state = ws.fileTree;
            if (!state.folder.empty()) {
                openFolder(state.folder);
            }
            
            auto& files = state.files;
            for (auto it = files.rbegin(); it != files.rend(); it++) {
                openFile(*it);
            }
        }
        void draw() {
            const auto regionX = ImGui::GetContentRegionAvail().x;
            const auto btnSize = ImVec2(regionX, 0);
            ImGui::SetNextItemWidth(regionX);
            ImGui::InputTextWithHint("##filter", "Search...", searchBuffer, 32);
            ImGui::Spacing();

            const bool hasFiles = !files.empty();
            const bool hasFolder = !folder.name.empty();
            if (hasFiles) {

                FileItem* itemForDelete = nullptr;
                ImGuiStyle& style = ImGui::GetStyle();
                auto group_start = ImGui::GetCursorPosX() + style.FramePadding.x;
                auto group_size = ImVec2(regionX - 2 * style.FramePadding.x, 0);
                auto btn_offset = group_size.x - style.ItemSpacing.x - style.FramePadding.x;

                ImGui::SetCursorPosX(group_start);
                ImGui::BeginGroup();
                for (int id = 0; auto& item : files) {
                    ImGui::PushID(id++);
                    ImGui::SetNextItemAllowOverlap();
                    ImGui::Selectable(item.name.c_str(), false, 0, group_size);
                    setDragDrop(&item.path, item.name);

                    ImGui::SameLine(btn_offset);
                    if (ImGui::SmallButton("X")) {
                        itemForDelete = &item;
                    }
                    ImGui::SetItemTooltip("Close");
                    ImGui::PopID();
                }
                ImGui::EndGroup();

                if (itemForDelete) {
                    files.remove_if([itemForDelete](FileItem& node) {
                        return itemForDelete == &node;
                    });
                }

            }

            if (hasFiles && hasFolder) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            if (hasFolder) {
                drawNode(folder);
            } else {
                ImGui::Spacing();
                if (ImGui::Button("Open Folder", btnSize)) { cmd::openFolder(); }
            }

            if (needCloseFolder) {
                needCloseFolder = false;
                closeFolder();
            }
        }
    private:
        void loadChildreen(TreeNode& parent) {
            parent.childreen.clear();
            for (const auto& child : fs::directory_iterator(parent.path)) {
                auto& childPath = child.path();
                TreeNode node;
                node.folder = fs::is_directory(childPath);
                node.loaded = false;
                node.expanded = false;
                node.path = childPath;
                node.name = toUTF8(childPath.filename());
                parent.childreen.push_back(node);
            }

            auto foldersOnTop = [](const TreeNode& left, const TreeNode& right) {
                return left.folder > right.folder;
            };
            std::stable_sort(parent.childreen.begin(), parent.childreen.end(), foldersOnTop);
        }
        void drawNode(TreeNode& node) {
            ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_DrawLinesNone;
            if (node.expanded) {
                base_flags |= ImGuiTreeNodeFlags_DefaultOpen;
            }

            bool isTopLevel = (&node == &folder);
            auto rx = isTopLevel ? ImGui::GetContentRegionAvail().x : 0.f;
            node.expanded = ImGui::TreeNodeEx(node.name.c_str(), base_flags);
            if (isTopLevel) {
                ImGui::SameLine(rx - ImGui::GetStyle().WindowPadding.x);
                if (ImGui::SmallButton("X")) {
                    needCloseFolder = true;
                }
                ImGui::SetItemTooltip("Close");
            }

            if (node.expanded) {
                if (node.folder && !node.loaded) {
                    node.loaded = true;
                    loadChildreen(node);
                }
                for (auto& child : node.childreen) {
                    if (child.folder) {
                        drawNode(child);
                    } else {
                        drawFile(child);
                    }
                }
                ImGui::TreePop();
            }
        }
        void drawFile(TreeNode& node) {
            ImGui::Selectable(node.name.c_str());
            setDragDrop(&node.path, node.name);
        }
        void setDragDrop(fs::path* path, string& tooltip) {
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                auto data = dragDrop::packPath(path);
                ImGui::SetDragDropPayload(dragDrop::PATH_TAG, &data, sizeof(data), ImGuiCond_Once);
                ImGui::Text(tooltip.c_str());
                ImGui::EndDragDropSource();
            }
        }
    };

    struct FrameController {
        Player& player;
        FrameRender& frameRender;
        FrameWindow& frameWindow;
        void linkChildreen();
        void update(const time_point& now);
        void openFile(const string& path);
        void closeFile();
        void togglePause();
        void seekLeft();
        void seekRight();
        void clearDrawn();
        void updateCursor();
    };

    enum SplitMode {
        Single = 0,
        HSplit = 1,
        VSplit = 2
    };

    enum WorkMode {
        MoveVideo = 0,
        DrawLines = 1
    };

    int splitMode = SplitMode::Single;
    int workMode = WorkMode::MoveVideo;
    bool openedColor = true;
    bool openedKeys = true;
    bool openedWorkspace = true;
    int drawLineWidth = 20;
    float drawLineColor[3] = { 1.0f, 0.0f, 0.0f };
    FrameController* seekTarget = nullptr;
    FrameWindow* singleModeTarget = nullptr;

    static void newFrame();
    static void render();
    static void draw();
    static void drawMainMenuBar(float& height);
    static void drawColorWindow();
    static void drawKeysWindow();
    static void setSplitMode(SplitMode mode);
    static void setSeekTarget(FrameController* target, bool hovered);
    static void setWorkMode(WorkMode mode);
    static void seekLeft();
    static void seekRight();
    static void togglePause();
    static void saveState(WorkState& ws);
    static void restoreState(const WorkState& ws);
}

Render render;
Player player0;
Player player1;
ui::MainWindow mainWindow;
ui::FileTreeWindow fileTreeWindow;
ui::FrameWindow frameWindow_0("Frame 0", "##frameWindow_0");
ui::FrameWindow frameWindow_1("Frame 1", "##frameWindow_1");
ui::FrameController fc[2] = {
    { player0, render.frames[0], frameWindow_0 },
    { player1, render.frames[1], frameWindow_1 }
};

static void ui::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
static void ui::draw() {  

    float menuHeight = 0.f;
    ui::drawMainMenuBar(menuHeight);

    auto workSize = ImGui::GetMainViewport()->WorkSize;
    auto winPadding = ImGui::GetStyle().WindowPadding;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowPos(ImVec2(0, menuHeight), ImGuiCond_Once);
    ImGui::SetNextWindowSize(workSize);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("Main", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_HorizontalScrollbar
    );

    
    //File Tree
    if(ui::openedWorkspace) {
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::SetNextWindowSizeConstraints(ImVec2(100, -1), ImVec2(workSize.x * 0.5, -1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, winPadding);
        ImGui::BeginChild("FileTree", ImVec2(250, 0),
            ImGuiChildFlags_Borders |
            ImGuiChildFlags_ResizeX,
            ImGuiWindowFlags_HorizontalScrollbar);

        fileTreeWindow.draw();

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::SameLine(0.f, 0.f);
    }
    
    auto sz = ImGui::GetContentRegionAvail();
    auto pos = ImGui::GetCursorScreenPos();
    ImGui::End();
    ImGui::PopStyleVar();

    switch (splitMode) {
    case SplitMode::Single: {
        if (singleModeTarget) {
            ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(sz.x, sz.y), ImGuiCond_Always);
            singleModeTarget->draw();
        }
        break;
    }
    case SplitMode::HSplit: {
        ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(0.5f * sz.x, sz.y), ImGuiCond_Always);
        frameWindow_0.draw();

        ImGui::SetNextWindowPos(ImVec2(pos.x + 0.5f * sz.x, pos.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(0.5 * sz.x, sz.y), ImGuiCond_Always);
        frameWindow_1.draw();
        break;
    }
    case SplitMode::VSplit: {
        ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(sz.x, sz.y * 0.5f), ImGuiCond_Always);
        frameWindow_0.draw();

        ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + sz.y * 0.5f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(sz.x, sz.y * 0.5), ImGuiCond_Always);
        frameWindow_1.draw();
        break;
    }
    }

    //ImGui::SetNextWindowPos(ImVec2(100, menuHeight + workSize.y - 300), ImGuiCond_FirstUseEver);
    ui::drawColorWindow();
    ui::drawKeysWindow();
}
static void ui::drawMainMenuBar(float& height) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open File", "[CTRL+O]")) { cmd::openFile(); }
            if (ImGui::MenuItem("Open Folder", "[CTRL+K, O]")) { cmd::openFolder(); }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "[ALT+Q, Q]")) { cmd::quitProgram(); }
            ImGui::EndMenu();
        }

        
        if (ImGui::BeginMenu("View")) {
            //ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
            //ImGui::PopItemFlag();
            if (ImGui::MenuItem("Workspace", nullptr, openedWorkspace)) { openedWorkspace = !openedWorkspace; }
            if (ImGui::MenuItem("Color", nullptr, openedColor)) { openedColor = !openedColor; }
            if (ImGui::MenuItem("Hot Keys", nullptr, openedKeys)) { openedKeys = !openedKeys; }   
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Split")) {
            if (ImGui::MenuItem("Single frame", nullptr, splitMode == SplitMode::Single)) { 
                ui::setSplitMode(SplitMode::Single);
            }
            if (ImGui::MenuItem("Left + Right", nullptr, splitMode == SplitMode::HSplit)) {
                ui::setSplitMode(SplitMode::HSplit);
            }
            if (ImGui::MenuItem("Top + Bottom", nullptr, splitMode == SplitMode::VSplit)) {
                ui::setSplitMode(SplitMode::VSplit);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Presets")) {
            if (ImGui::MenuItem("Landscape [1280x720]")) { mainWindow.setSize(1280, 720); }
            if (ImGui::MenuItem("Portrait [720x1280]")) { mainWindow.setSize(720, 1280); }
            if (ImGui::MenuItem("Square [1080x1080]")) { mainWindow.setSize(1080, 1080); }
            if (ImGui::MenuItem("Square [720x720]")) { mainWindow.setSize(960, 960); }
            ImGui::EndMenu();
        }

        height = ImGui::GetWindowSize().y;
        ImGui::EndMainMenuBar();
    }
}
static void ui::drawColorWindow() {
    if (!ui::openedColor) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Color", &ui::openedColor)) {
        bool changed = ImGui::DragInt("Width", &ui::drawLineWidth, 0.5f, 2, 50);
        changed |= ImGui::ColorEdit3("Color", ui::drawLineColor);
        if (changed) {
            fc[0].frameRender.setBrush(ui::drawLineColor, ui::drawLineWidth); //todo: render.setBrush() instead of framecontroller?
            fc[1].frameRender.setBrush(ui::drawLineColor, ui::drawLineWidth); //todo: render.setBrush() --//-- ?
        }
    }
    ImGui::End();
}
static void ui::drawKeysWindow() {
    if (!ui::openedKeys) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(0, 0));
    if (ImGui::Begin("Hot Keys", &ui::openedKeys)) {
        ImGui::Text("Play/pause"); ImGui::SameLine(); ImGui::TextDisabled("[SPACE]");
        ImGui::Text("Next frame"); ImGui::SameLine(); ImGui::TextDisabled("[RIGHT] or [D]");
        ImGui::Text("Prev frame"); ImGui::SameLine(); ImGui::TextDisabled("[LEFT] or [A]");

        ImGui::Separator();
        ImGui::Text("Move video"); ImGui::SameLine(); ImGui::TextDisabled("Mouse LEFT");
        ImGui::Text("Zoom video"); ImGui::SameLine(); ImGui::TextDisabled("Mouse WHEEL");

        ImGui::Separator();
        ImGui::Text("Clear drawn"); ImGui::SameLine(); ImGui::TextDisabled("[ESC]");
        ImGui::Text("Draw points"); ImGui::SameLine(); ImGui::TextDisabled("[ALT] + Mouse LEFT");
        ImGui::Text("Draw lines"); ImGui::SameLine(); ImGui::TextDisabled("[ALT] + Mouse RIGHT");
        ImGui::Text("Set width"); ImGui::SameLine(); ImGui::TextDisabled("[ALT] + Mouse WHEEL");
    }
    ImGui::End();
}
static void ui::render() {
    //ImGui::ShowDemoWindow();
    //ImGui::DebugTextEncoding("Привет");
    //ImGui::ShowMetricsWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
static void ui::setSplitMode(SplitMode mode) {
    splitMode = mode;
    singleModeTarget = nullptr;
    if (mode == SplitMode::Single) {
        if (fc[1].player.ps.started) {
            singleModeTarget = &fc[1].frameWindow;
            fc[0].closeFile();
        } else {
            singleModeTarget = &fc[0].frameWindow;
            fc[1].closeFile();
        }
    }
}
static void ui::setSeekTarget(FrameController* target, bool hovered) {
    if (hovered) {
        seekTarget = target;
    }
    else if (seekTarget == target) {
        seekTarget = nullptr;
    }
}
static void ui::setWorkMode(WorkMode mode) {
    workMode = mode;
    cout << "set work mode " << workMode << endl;
}
static void ui::seekLeft() {
    if (ui::seekTarget) {
        ui::seekTarget->seekLeft();
    }
    else {
        fc[0].seekLeft(); //todo: longseek?
        fc[1].seekLeft();
    }
    
}
static void ui::seekRight() {
    if (ui::seekTarget) {
        ui::seekTarget->seekRight();
    } else {
        fc[0].seekRight();
        fc[1].seekRight();
    }
}
static void ui::togglePause() {
    if (ui::seekTarget) {
        ui::seekTarget->togglePause();
    }
    else {
        auto& p0 = fc[0].player;
        auto& p1 = fc[1].player;
        bool paused = !p0.ps.paused || !p1.ps.paused;
        if (!p0.eof()) {
            p0.pause(paused);
        }
        if (!p1.eof()) {
            p1.pause(paused);
        }
    }
}
static void ui::saveState(WorkState& ws) {
    ws.openedColor = ui::openedColor;
    ws.openedKeys = ui::openedKeys;
    ws.openedWorkspace = ui::openedWorkspace;
    ws.splitMode = ui::splitMode;
    ws.drawLineWidth = ui::drawLineWidth;
    ws.drawLineColor[0] = ui::drawLineColor[0];
    ws.drawLineColor[1] = ui::drawLineColor[1];
    ws.drawLineColor[2] = ui::drawLineColor[2];
}
static void ui::restoreState(const WorkState& ws) {
    ui::openedColor         = ws.openedColor;
    ui::openedKeys          = ws.openedKeys;
    ui::openedWorkspace     = ws.openedWorkspace;
    ui::splitMode           = ws.splitMode;
    ui::drawLineWidth       = ws.drawLineWidth;
    ui::drawLineColor[0]    = ws.drawLineColor[0];
    ui::drawLineColor[1]    = ws.drawLineColor[1];
    ui::drawLineColor[2]    = ws.drawLineColor[2];

    if (splitMode == SplitMode::Single) {
        singleModeTarget = &fc[0].frameWindow;
    }
}


static void cmd::openFile() {
    string path;
    if (nfd::showNativeFileDialog(path, false)) {
        fileTreeWindow.openFile(path);
    }
}
static void cmd::openFolder() {
    string path;
    if (nfd::showNativeFileDialog(path, true)) {
        fileTreeWindow.openFolder(path);
    }
}
static void cmd::quitProgram() {
    mainWindow.close();
}
static void reshapeCallback(GLFWwindow*, int w, int h) {
    mainWindow.reshape(w, h);
}
static void mouseCallback(FrameRender& frame, int mx, int my) {

    ImGuiIO& io = ImGui::GetIO();
    const int& dx = io.MouseDelta.x;
    const int& dy = io.MouseDelta.y;
    const bool hasDelta = dx || dy;

    if (ui::workMode == ui::WorkMode::MoveVideo) {

        if (io.MouseWheel) {
            bool ctrl = ImGui::IsKeyDown(ImGuiMod_Ctrl);
            float value = ctrl ? (io.MouseWheel * 0.25f) : io.MouseWheel;
            frame.zoomCam(value);
        }

        if (hasDelta) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                frame.moveCam(dx, dy);
            } else {
                frame.moveCursor(mx, my);
            }
        }
    }
    else if (ui::workMode == ui::WorkMode::DrawLines) {

        if (io.MouseWheel) {
            int delta = std::max(1.f, ui::drawLineWidth * 0.2f);
            ui::drawLineWidth += io.MouseWheel * delta;
            frame.setBrush(ui::drawLineColor, ui::drawLineWidth);
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            
            DrawType drawMode =
                ImGui::IsMouseDown(ImGuiMouseButton_Left) ? DrawType::Points :
                ImGui::IsMouseDown(ImGuiMouseButton_Right) ? DrawType::Segments : 
                DrawType::None;
            frame.drawStart(mx, my, drawMode);
        }
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            frame.drawStop(mx, my);

            auto mode = ImGui::IsKeyDown(ImGuiKey_LeftAlt) ?
                ui::WorkMode::DrawLines : ui::WorkMode::MoveVideo;
            ui::setWorkMode(mode);
            fc[0].updateCursor();
            fc[1].updateCursor();
        }
        else if (hasDelta) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right)){
                frame.drawNext(mx, my);
            }
            frame.moveCursor(mx, my);
        }
    }
}
static void keyCallback(GLFWwindow* window, int keyCode, int scanCode, int action, int mods) {
    using namespace io;
    using namespace io::keyboard;

    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    
    const KeyEvent& key = keyboard::create(keyCode, action, mods);
    if (key.pressed(R)) {
        std::cout << "Reload shaders" << std::endl;
        render.reloadShaders();
    }
    else if (key.pressed(SPACE)) {
        ui::togglePause();
    }
    else if (key.pressed(LEFT) || key.pressed(A)) {
        ui::seekLeft();
    }
    else if (key.pressed(RIGHT) || key.pressed(D)) {
        ui::seekRight();
    }
    else if (key.pressed(ESC)) {
        fc[0].clearDrawn();
        fc[1].clearDrawn();
    }
    else if (key.is(Mod::ALT, Q, Q)) {
        cmd::quitProgram();
    }
    else if (key.is(Mod::CONTROL, K, O)) {
        cmd::openFolder();
    }
    else if (key.is(Mod::CONTROL, O)) {
        cmd::openFile();
    }
    else if (key.is(LEFT_ALT) && action != Action::REPEAT) {
        auto mode = action == Action::PRESS ?
            ui::WorkMode::DrawLines :
            ui::WorkMode::MoveVideo;
        ui::setWorkMode(mode);
        fc[0].updateCursor();
        fc[1].updateCursor();
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
    glfwSetFramebufferSizeCallback(window, reshapeCallback);
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
static void saveWorkspace() {
    WorkState ws;
    mainWindow.saveState(ws);
    fileTreeWindow.saveState(ws);
    ui::saveState(ws);
    ws.save(resources::workspace);
}
static void loadWorkspace() {

    WorkState ws;
    if (ws.load(resources::workspace)) {
        mainWindow.restoreState(ws);
        fileTreeWindow.restoreState(ws);
        ui::restoreState(ws);
    }

    //todo: make more clean


    render.frames[0].setBrush(ui::drawLineColor, ui::drawLineWidth);
    render.frames[1].setBrush(ui::drawLineColor, ui::drawLineWidth);
}

void ui::FrameController::linkChildreen() {
    frameWindow.hoverFrameFn = [this](bool hovered) {
        frameRender.drawReset();
        frameRender.showCursor(hovered && (ui::workMode == DrawLines) && ImGui::IsKeyDown(ImGuiKey_LeftAlt));
    };
    frameWindow.hoverSlideFn = [this](bool hovered) {
        ui::setSeekTarget(this, hovered);
    };
    frameWindow.mouseFn = [this](int mx, int my) {
        mouseCallback(frameRender, mx, my);
    };
    frameWindow.slideFn = [this](float progress, bool hold) {
        player.seekProgress(progress, hold);
    };
    frameWindow.reshapeFn = [this](const ImVec2& size) {
        frameRender.reshape(size.x, size.y);
    };
    frameWindow.acceptDropFn = [this](const string& path) {
        this->openFile(path);
    };
    frameWindow.closeFn = [this]() {
        this->closeFile();
    };
}
void ui::FrameController::update(const time_point& now) {
   
    if (player.hasUpdate(now)) {
        const RGBFrame* rgb = player.currentFrame();
        if (rgb) {
            frameRender.updateTexture(rgb->width, rgb->height, rgb->pixels);
        }
        frameWindow.setProgress(player.ps.progress);

        //todo: do this after two updates
        if (player.eof() && ui::splitMode != SplitMode::Single && ui::seekTarget == nullptr) {
            if (&player == &fc[0].player) {
                fc[1].player.pause(true);
            }
            if (&player == &fc[1].player) {
                fc[0].player.pause(true);
            }
        }
    }
    

}
void ui::FrameController::openFile(const string& path) {
    if (player.start(path.c_str())) {
        const auto fileName = fs::path(path).filename().string();
        const auto& info = player.info;
        frameRender.clearDrawn();
        frameRender.createTexture(info.width, info.height);
        frameWindow.setTextureID(frameRender.fb.tid);
        frameWindow.setName(fileName.c_str());
        cout << "File open - ok: " << path << endl;
    }
    else {
        std::cout << "File open - error" << std::endl;
    }
}
void ui::FrameController::closeFile() {
    player.stop();
    frameRender.clearDrawn();
    frameWindow.setTextureID(ImTextureID_Invalid);
}
void ui::FrameController::togglePause() {
    bool newValue = !(player.ps.paused);
    player.pause(newValue);
}
void ui::FrameController::seekLeft() {
    player.seekLeft();
}
void ui::FrameController::seekRight() {
    player.seekRight();
}
void ui::FrameController::clearDrawn() {
    frameRender.clearDrawn();
}
void ui::FrameController::updateCursor() {
    frameRender.showCursor(frameWindow.frameHovered && (ui::workMode == DrawLines));
}

int main(int argc, char* argv[]) {
    if (!glfwInit()) {
        std::cout << "glfwInit error" << std::endl;
        return -1;
    }

    loadWorkspace();
    
    GLFWwindow* window = mainWindow.create();
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
    fc[0].linkChildreen();
    fc[1].linkChildreen();

    while (!glfwWindowShouldClose(window)) {

        auto now = steady_clock::now();
        fc[0].update(now);
        fc[1].update(now); 

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ui::newFrame();
        ui::draw();
        render.renderFrames();
        ui::render();

        glFlush();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    saveWorkspace();
    player0.stop();
    player1.stop();
    render.destroyFrames();
    render.destroyShaders();
    render.destroyFrameBuffers();
    destroyImGui();
    glfwTerminate();
	return 0;
}
