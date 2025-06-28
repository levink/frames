#version 120
precision highp float;

uniform mat4 Proj;
uniform mat4 View;

//#vertex
attribute vec2 in_Position;
attribute vec2 in_Center;
void main() {
    TexCoord = in_Texture;
    gl_Position = Proj * View * vec4(in_Position, 0.0, 1.0);
}

//#fragment
void main() {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}