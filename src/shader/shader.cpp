#include "shader.h"

VideoShader::VideoShader() : Shader(3, 2) {
    u[0] = Uniform("VideoTexture");
    u[1] = Uniform("Proj");
    u[2] = Uniform("View");
    a[0] = Attribute(VEC_2, "in_Position");
    a[1] = Attribute(VEC_2, "in_Texture");
}
void VideoShader::enable() const {
    Shader::enable();
    glEnable(GL_TEXTURE_2D);
}
void VideoShader::disable() const {
    Shader::disable();
    glDisable(GL_TEXTURE_2D);
}
void VideoShader::draw(const FrameRender& frame) {
    glBindTexture(GL_TEXTURE_2D, frame.textureId);
    set1(u[0], 0);
    set4(u[1], frame.cam.proj);
    set4(u[2], frame.cam.view);
    attr(a[0], frame.mesh.position);
    attr(a[1], frame.mesh.texture);
    drawFaces(frame.mesh.face);
    glBindTexture(GL_TEXTURE_2D, 0);
}

CircleShader::CircleShader() : Shader(2, 3) {
    u[0] = Uniform("Proj");
    u[1] = Uniform("View");
    a[0] = Attribute(VEC_2, "in_Position");
    a[1] = Attribute(VEC_2, "in_Center");
    a[2] = Attribute(FLOAT, "in_Radius");
}
void CircleShader::enable() const {
    Shader::enable();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void CircleShader::disable() const {
    Shader::disable();
    glDisable(GL_BLEND);
}
void CircleShader::draw(const FrameRender& frame) {
    set4(u[0], frame.cam.proj);
    set4(u[1], frame.cam.view);
    attr(a[0], frame.circles.position);
    attr(a[1], frame.circles.center);
    attr(a[2], frame.circles.radius);
    drawFaces(frame.circles.face);
}