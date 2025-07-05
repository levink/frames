#pragma once
#include <string>
#include <vector>

struct MainWindowState {
    uint32_t width = 0;
    uint32_t height = 0;
};

struct FolderWindowState {
    std::string folder;
    std::vector<std::string> files;
};

struct Workspace {
    const uint32_t formatVersion = 2;
    MainWindowState mainWindow;
    FolderWindowState folderWindow;

    void save(const char* path);
    bool load(const char* path);
};
