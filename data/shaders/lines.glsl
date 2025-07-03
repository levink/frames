#version 120
precision highp float;

uniform mat4 Proj;
uniform mat4 View;
uniform vec3 Color;
uniform float Offset;
uniform float Scale;

varying vec2 Position;
varying vec2 LineStart;
varying vec2 LineEnd;
varying float Radius;

//#vertex
attribute vec2 in_Position;
attribute vec2 in_LineStart;
attribute vec2 in_LineEnd;
attribute float in_Radius;

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
        // distance from point
        return distance(start, point);
    }

    // distance from segment
    float length = dir.x * dir.x + dir.y * dir.y;
    float t = max(0.0, min(1.0, dot(point - start, dir) / length)); 
    vec2 proj = start + t * dir;
    return distance(point, proj);
}
void main() {
    float inner = Radius + (Offset - 1.5) * Scale;
    float outer = Radius + Offset * Scale;
    float dist = getDistance(LineStart, LineEnd, Position);
    float alpha = 1.0 - smoothstep(inner, outer, dist);
    gl_FragColor = vec4(Color, alpha);
}
