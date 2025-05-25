#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

struct GLFace {
    uint16_t a = 0;
    uint16_t b = 0;
    uint16_t c = 0;
};

struct Mesh {
    std::vector<glm::vec2> position;
    std::vector<glm::vec2> texture;
    std::vector<GLFace> face;
    glm::mat4 modelMatrix   = glm::mat4(1);
    glm::fvec2 size         = { 0, 0 };
    glm::fvec3 offset       = { 0, 0, 0 };
    glm::fvec3 scale        = { 1, 1, 1 };

    void setSize(int w, int h);
    void move(int deltaX, int deltaY);
    void zoom(float value);

private:
    void updateMatrix();
};
