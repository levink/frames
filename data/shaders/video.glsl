#version 120
precision highp float;

uniform mat4 Ortho;
// uniform sampler2D TextureId;
// varying vec2 TexCoord;

//#vertex
attribute vec2 in_Position;
// attribute vec2 in_Texture;
void main() {
    gl_Position = Ortho * vec4(in_Position, 0.0, 1.0);
}

//#fragment
void main() {
    // vec3 a1 = texture2D(Texture1, TexCoord).a;
    gl_FragColor = vec4(1.0, 0.4, 0.0, 1.0);
}