#pragma once
#include <glm/glm.hpp>

struct ViewPort {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct Camera {
    ViewPort vp;
    glm::mat4 ortho;
    void reshape(int x, int y, int w, int h);
};