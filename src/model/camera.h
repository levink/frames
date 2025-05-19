#pragma once
#include <glm/glm.hpp>

struct Camera {
    glm::ivec2 viewOffset;
    glm::ivec2 viewSize;
    glm::mat4 Ortho;
    void reshape(int x, int y, int width, int height);
    void scale(float value);
};