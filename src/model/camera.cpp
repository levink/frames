#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"

void Camera::reshape(int w, int h) {
    float halfWidth = std::clamp(w / 2, 0, 2048);
    float halfHeight = std::clamp(h / 2, 0, 2048);
    proj = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
    updateMatrix();
}

void Camera::init(const glm::vec2& imageCenter, float zoom) {
    degrees = 0;
    radians = 0.f;
    pivot.x = imageCenter.x;
    pivot.y = imageCenter.y;
    offset = { -pivot.x, -pivot.y, 0 };
    scale = { zoom, zoom, 1 };
    updateMatrix();
}

void Camera::move(int dx, int dy) {
    offset.x += dx / scale.x;
    offset.y += dy / scale.y;
    updateMatrix();
}

void Camera::zoom(float value) {
    value *= scale.x; //adaptive zoom
    scale.x += value;
    scale.y += value;
    updateMatrix();
}

void Camera::rotate(float deltaDegrees) {
    constexpr float DEG_TO_RAD = 3.1415926f / 180.f;
    degrees += deltaDegrees;
    degrees = degrees % 360;
    radians = degrees * DEG_TO_RAD;
    updateMatrix();
}

void Camera::updateMatrix() {
  
    view = glm::scale(glm::mat4(1), scale);
    view = glm::translate(view, offset);
    
    view = glm::translate(view, pivot);
    view = glm::rotate(view, radians, { 0, 0, -1 });
    view = glm::translate(view, -pivot);
    
    pv_inverse = glm::inverse(proj * view);
    scale_inverse = std::abs(scale.x) < 0.01f ? 
        (1.f) :
        (1.f / scale.x);
}
