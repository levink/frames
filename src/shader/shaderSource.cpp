#include <fstream>
#include "shaderSource.h"
#include "shaderLog.h"

namespace util {
    static void swap(ShaderSource& left, ShaderSource& right) {
        std::swap(left.vertex, right.vertex);
        std::swap(left.fragment, right.fragment);
    }
    static std::string getShaderText(const char* fileName) {
        std::string result;
        std::ifstream input(fileName, std::ifstream::binary);
        if (!input) {
            ShaderLog::warn("Could not open file", fileName);
            return result;
        }

        input.seekg(0, std::ios::end);
        size_t file_length = input.tellg();
        input.seekg(0, std::ios::beg);

        char* buf = new char[file_length + 1];
        std::streamsize readCount = 0;
        while (readCount < file_length) {
            input.read(buf + readCount, file_length - readCount);
            readCount += input.gcount();
        }

        buf[file_length] = 0;
        input.close();

        result.assign(buf);
        delete[] buf;

        return std::move(result);
    }
}

ShaderSource::ShaderSource(const ShaderSource& right) {
    vertex = right.vertex;
    fragment = right.fragment;
}
ShaderSource::ShaderSource(ShaderSource&& right) noexcept : ShaderSource() {
    util::swap(*this, right);
}
ShaderSource& ShaderSource::operator=(const ShaderSource& right) {
    if (this == &right) {
        return *this;
    }

    vertex = right.vertex;
    fragment = right.fragment;
    return *this;
}
ShaderSource& ShaderSource::operator=(ShaderSource&& right) noexcept {
    if (this == &right) {
        return *this;
    }

    util::swap(*this, right);
    return *this;
}

void ShaderSource::load(const char* path) {
    std::string text = util::getShaderText(path);

    static const auto vertexTag = "//#vertex";
    auto vertexPosition = text.find(vertexTag);
    if (vertexPosition == std::string::npos) {
        ShaderLog::warn("Vertex tag not found in shader", path);
        return;
    }

    static const auto fragmentTag = "//#fragment";
    auto fragmentPosition = text.find(fragmentTag);
    if (fragmentPosition == std::string::npos) {
        ShaderLog::warn("Fragment tag not found in shader", path);
        return;
    }

    auto uniforms = text.substr(0, vertexPosition);
    this->vertex = text.substr(0, fragmentPosition);
    this->fragment = uniforms + text.substr(fragmentPosition);
}