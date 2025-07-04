#pragma once
#include <string>
#include <vector>


struct FolderWindowState {
    std::string folder;
    std::vector<std::string> files;
};

struct Workspace {
    const uint32_t formatVersion = 1;
    FolderWindowState folderWindow;

    void save(const char* path);
    bool load(const char* path);
};
