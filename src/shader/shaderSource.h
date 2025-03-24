#pragma once
#include <string>

namespace shader_files {
    static const char* video = "../../data/shaders/video.glsl";
}

struct ShaderSource {
    std::string vertex;
    std::string fragment;
    ShaderSource() = default;
    ShaderSource(const ShaderSource& right);
    ShaderSource(ShaderSource&& right) noexcept;
    ShaderSource& operator=(const ShaderSource& right);
    ShaderSource& operator=(ShaderSource&& right) noexcept;
    void load(const char* path);
};
