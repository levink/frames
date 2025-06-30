#version 120
precision highp float;

uniform mat4 Proj;
uniform mat4 View;
// uniform vec3 Color;
// uniform float Size;

//#vertex
attribute vec2 in_Position;
void main() {
	gl_Position = Proj * View * vec4(in_Position, 1.0, 1.0);
	gl_PointSize = 4.0;//Size;
}

//#fragment
void main() {
	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}