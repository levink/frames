#pragma once
#include <glm/glm.hpp>

struct ViewPort {
    int left = 0;
    int bottom = 0;
    int width = 0;
    int height = 0;
    bool hit(int x, int y) const;
};

struct Camera {
    ViewPort vp;
    glm::mat4 ortho;
    void reshape(int x, int y, int w, int h);
};
