#pragma once
#include <glm/glm.hpp>

struct Camera {
    glm::ivec2 viewSize;
    glm::mat4 Ortho;
    void reshape(int w, int h);
};