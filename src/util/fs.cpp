#include "fs.h"

std::string toUTF8(const fs::path& path) {
    auto utf8 = path.u8string();
    return std::move(std::string(utf8.begin(), utf8.end()));
}