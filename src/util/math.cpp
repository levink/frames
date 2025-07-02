#include "math.h"
using glm::ivec2;
using glm::vec2;

float math::distanceL1(const glm::vec2& left, const glm::vec2& right) {
    const float dx = abs(left.x - right.x);
    const float dy = abs(left.y - right.y);
    return dx + dy;
}

glm::vec2 math::dir(const glm::vec2& from, const glm::vec2& to) {
    return glm::normalize(to - from);
}