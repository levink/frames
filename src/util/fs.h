#pragma once 

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

std::string toUTF8(const fs::path& path);
