#pragma once
#include <string>
#include <vector>

struct MainState {
    uint32_t width = 0;
    uint32_t height = 0;
};

struct FileTreeState {
    std::string folder;
    std::vector<std::string> files;
};

struct WorkState {
    const uint32_t formatVersion = 3;
    MainState main;
    FileTreeState fileTree;

    bool openedColor = true;
    bool openedKeys = true;
    bool openedWorkspace = true;

    uint8_t splitMode = 0;
    uint32_t drawLineWidth = 20;
    float drawLineColor[3] = { 1.0f, 0.0f, 0.0f };

    void save(const char* path);
    bool load(const char* path);
};
