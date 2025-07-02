#pragma once
#include <glm/glm.hpp>

namespace math {
    float distanceL1(const glm::vec2& left, const glm::vec2& right);
    glm::vec2 dir(const glm::vec2& from, const glm::vec2& to);
}

//static float radiansBetween(glm::vec2 a, glm::vec2 b) {
	/*
	Угол между векторами.
	Если результат положительный, значит "a" нужно вращать против часовой стрелки для совмещения с "b",
	иначе - по часовой.
*/
/*a = normalize(a);
b = normalize(b);
const float value = dot(a, b);
float radians = acos(value);

const vec2 normal = vec2(-a.y, a.x);
if (dot(b, normal) > 0) {
	radians = -radians;
}

return radians;*/
//	return 0.0;
//}