#include "shader.h"

VideoShader::VideoShader() : Shader(3, 2) {
    u[0] = Uniform("VideoTexture");
    u[1] = Uniform("Ortho");
    u[2] = Uniform("Model");
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
    const auto& vp = frame.vp;
    glViewport(vp.x, vp.y, vp.w, vp.h);
    glBindTexture(GL_TEXTURE_2D, frame.textureId);
    set1(u[0], 0);
    set4(u[1], frame.cam.ortho);
    set4(u[2], frame.mesh.modelMatrix);
    attr(a[0], frame.mesh.position);
    attr(a[1], frame.mesh.texture);
    drawFaces(frame.mesh.face);
    glBindTexture(GL_TEXTURE_2D, 0);
}
