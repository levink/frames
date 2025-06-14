#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "camera.h"

void Camera::reshape(int w, int h) {
    float halfWidth = std::clamp(w / 2, 0, 2048);
    float halfHeight = std::clamp(h / 2, 0, 2048);
    ortho = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight);
}
