#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include "camera.h"

void Camera::reshape(int width, int height) {
    viewSize.x = std::clamp(width, 0, 4096);
    viewSize.y = std::clamp(height, 0, 4096);
    Ortho = glm::ortho(
        0.f, static_cast<float>(viewSize.x),
        0.f, static_cast<float>(viewSize.y));
    glViewport(0, 0, viewSize.x, viewSize.y);
}