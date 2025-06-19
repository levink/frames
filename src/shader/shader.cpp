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
void VideoShader::draw(const FrameView& frame) {
    setViewPort(frame.viewPort, frame.viewSize);
    glBindTexture(GL_TEXTURE_2D, frame.textureId);
    set1(u[0], 0);
    set4(u[1], frame.cam.proj);
    set4(u[2], frame.cam.view);
    attr(a[0], frame.mesh.position);
    attr(a[1], frame.mesh.texture);
    drawFaces(frame.mesh.face);
    glBindTexture(GL_TEXTURE_2D, 0);
}
