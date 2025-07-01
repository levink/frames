#version 120
precision highp float;

uniform mat4 Proj;
uniform mat4 View;

varying vec2 Position;
varying vec2 LineStart;
varying vec2 LineEnd;
varying float Radius;

//#vertex
attribute vec2 in_Position;
attribute vec2 in_LineStart;
attribute vec2 in_LineEnd;
attribute float in_Radius;
attribute vec3 in_Color;

void main() {
    gl_Position = Proj * View * vec4(in_Position, 0.0, 1.0);
    Position = in_Position; 
	LineStart = in_LineStart;
    LineEnd = in_LineEnd;
	Radius = in_Radius;
}

//#fragment
float getDistance(vec2 start, vec2 end, vec2 point) {
    vec2 dir = end - start;
    if (abs(dir.x) + abs(dir.y) < 0.01) {
        return distance(start, point);
    }

    //distance from segment
    float length = dir.x * dir.x + dir.y * dir.y;
    float t = max(0.0, min(1.0, dot(point - start, dir) / length));
    vec2 proj = start + t * dir;
    return distance(point, proj);
}
void main() {
    float inner = Radius - 3.5;
    float outer = Radius;
    float dist = getDistance(LineStart, LineEnd, Position);
    // float alpha = 0.5;
    float alpha = smoothstep(outer, inner, dist);
    gl_FragColor = vec4(1.0, 1.0, 0.4, alpha);
}

// void main() {
// 	const float width = 3.0;
// 	float inner = Radius - width;
// 	float middle = Radius - 0.5 * width;
// 	float dist = distance(Center, Position);
// 	float alpha = (Fill == 1.0 || dist > middle) ?
// 		smoothstep(Radius, middle, dist) :
// 		smoothstep(inner, middle, dist);
// 	gl_FragColor = vec4(Color.rgb, alpha);
// }