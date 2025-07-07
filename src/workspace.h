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

struct Workspace {
    const uint32_t formatVersion = 2;
    MainState main;
    FileTreeState fileTree;

    void save(const char* path);
    bool load(const char* path);
};
