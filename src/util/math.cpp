#include "math.h"
using glm::ivec2;
using glm::vec2;


float math::distance2(const vec2& left, const vec2& right) {
    const float dx = left.x - right.x;
    const float dy = left.y - right.y;
    return dx * dx + dy * dy;
}
