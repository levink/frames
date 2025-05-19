#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include "camera.h"

void Camera::reshape(int x, int y, int width, int height) {
    viewOffset.x = x;
    viewOffset.y = y;
    viewSize.x = std::clamp(width, 0, 4096);
    viewSize.y = std::clamp(height, 0, 4096);

    float dx = static_cast<float>(viewSize.x) / 2.f;
    float dy = static_cast<float>(viewSize.y) / 2.f;
    Ortho = glm::ortho(-dx, dx, -dy, dy);
}

void Camera::scale(float value) {
    Ortho = glm::scale(Ortho, { value, value, value });
}