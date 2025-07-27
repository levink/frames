#pragma once
#include <glm/glm.hpp>

namespace math {
    float distanceL1(const glm::vec2& left, const glm::vec2& right);
    glm::vec2 dir(const glm::vec2& from, const glm::vec2& to);
}
