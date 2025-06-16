#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"

void Camera::reshape(int w, int h) {
    float halfWidth = std::clamp(w / 2, 0, 2048);
    float halfHeight = std::clamp(h / 2, 0, 2048);
    proj = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
    updateMatrix();
}

void Camera::init(const glm::vec2& position, float zoom) {
    offset = { position.x, position.y, 0 };
    scale = { zoom, zoom, 1 };
    updateMatrix();
}

void Camera::move(int dx, int dy) {
    offset.x += dx / scale.x;
    offset.y += dy / scale.y;
    updateMatrix();
}

void Camera::zoom(float value) {
    scale.x += value;
    scale.y += value;
    updateMatrix();
}

void Camera::updateMatrix() {
    view = glm::scale(glm::mat4(1), scale);
    view = glm::translate(view, offset);
    pv_inverse = glm::inverse(proj * view);
}
