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
    GLuint textureId = 0;
    bool textureReady = false;
    std::vector<glm::vec2> position;
    std::vector<glm::vec2> texture;
    std::vector<GLFace> face;
    static ImageMesh createImageMesh(int w, int h);
};

struct LineVertex {
    glm::vec2 position;
    glm::vec2 segmentP0;
    glm::vec2 segmentP1;
    float radius = 0.f;
};

struct LineMesh {
    std::vector<LineVertex> vertex;
    std::vector<GLFace> face;
    glm::vec3 color;
    void reserveQuad();
    bool empty() const;
    void clear();
    void createPoint(size_t vertexOffset, const glm::vec2& pos, float radius);
    size_t offset() const;
};
