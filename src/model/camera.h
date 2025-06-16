#pragma once
#include <glm/glm.hpp>

struct Camera {
    glm::vec3 offset;
    glm::vec3 scale;
    glm::mat4 proj;
    glm::mat4 view;
    glm::mat4 pv_inverse;

    void reshape(int w, int h);
    void init(const glm::vec2& position, float zoom);
    void move(int dx, int dy);
    void zoom(float value);

private:
    void updateMatrix();
};
