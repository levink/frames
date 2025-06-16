#version 120
precision highp float;

uniform sampler2D VideoTexture;
uniform mat4 Proj;
uniform mat4 View;
varying vec2 TexCoord;

//#vertex
attribute vec2 in_Position;
attribute vec2 in_Texture;
void main() {
    TexCoord = in_Texture;
    gl_Position = Proj * View * vec4(in_Position, 0.0, 1.0);
}

//#fragment
void main() {
    gl_FragColor = texture2D(VideoTexture, TexCoord);
}