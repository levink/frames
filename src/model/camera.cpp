#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include "camera.h"

void Camera::reshape(int x, int y, int w, int h) {
    w = std::clamp(w, 0, 4096);
    h = std::clamp(h, 0, 4096);
    vp = ViewPort{ x, y, w, h };

    const float halfWidth = w * 0.5f;
    const float halfHeight = h * 0.5f;
    ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
}