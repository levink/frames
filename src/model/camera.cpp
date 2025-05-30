#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include "camera.h"

bool ViewPort::hit(int x, int y) const {
    if (x < left || left + width < x) {
        return false;
    }
    if (y < bottom || bottom + height < y) {
        return false;
    }
    return true;
}

void Camera::reshape(int x, int y, int w, int h) {
    w = std::clamp(w, 0, 4096);
    h = std::clamp(h, 0, 4096);
    vp = ViewPort{ x, y, w, h };

    const float halfWidth = w * 0.5f;
    const float halfHeight = h * 0.5f;
    ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
}