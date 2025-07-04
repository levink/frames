#pragma once
#include <string>
#include <vector>


struct FolderWindowState {
    std::string folder;
    std::vector<std::string> files;
};

struct Settings {
    const uint32_t formatVersion = 2;
    FolderWindowState folderWindow;

    void save(const char* path);
    void load(const char* path);
};
