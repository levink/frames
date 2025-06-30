#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

struct GLFace {
    uint16_t a = 0;
    uint16_t b = 0;
    uint16_t c = 0;
};

struct ImageMesh {
    std::vector<glm::vec2> position;
    std::vector<glm::vec2> texture;
    std::vector<GLFace> face;
    static ImageMesh createImageMesh(int w, int h);
};

struct LineVertex {
    glm::vec2 position;
    glm::vec2 start;
    glm::vec2 end;
    float radius;
};

struct LineMesh {
    std::vector<LineVertex> vertex;
    std::vector<GLFace> face;
    LineMesh();
    void addQuad(const glm::vec2& center, float radius);
};
