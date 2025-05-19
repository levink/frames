#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

struct GLFace {
    uint16_t a = 0;
    uint16_t b = 0;
    uint16_t c = 0;
};

struct VideoMesh {
    std::vector<glm::vec2> position;
    std::vector<glm::vec2> texture;
    std::vector<GLFace> face;
    glm::mat4 modelMatrix;
    void setSize(int w, int h);
    void move(int dx, int dy);
    void scale(float value);
};
