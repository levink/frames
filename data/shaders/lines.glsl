#version 120
precision highp float;

uniform mat4 Proj;
uniform mat4 View;
uniform vec3 Color;
uniform float Offset;

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

    //todo: 
    // 1. precompute value "1/lenght"
    // 2. avoid sqrt. calc distance square instead of distance and use it for calc alpha

    float inner = Radius + Offset - 1.5;
    float outer = Radius + Offset;
    float dist = getDistance(LineStart, LineEnd, Position);
    float alpha = smoothstep(outer, inner, dist);
    gl_FragColor = vec4(Color, alpha);
}
