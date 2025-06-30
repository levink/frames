#include "math.h"
using glm::ivec2;
using glm::vec2;


namespace math {

    int dist2(const ivec2& left, const ivec2& right) {
        const int dx = left.x - right.x;
        const int dy = left.y - right.y;
        return dx * dx + dy * dy;
    }
    float dist2(const vec2& left, const vec2& right) {
        const float dx = left.x - right.x;
        const float dy = left.y - right.y;
        return dx * dx + dy * dy;
    }
}