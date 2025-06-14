#pragma once
#include <glm/glm.hpp>

struct Camera {
    glm::mat4 ortho;
    void reshape(int w, int h);
};
