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
void VideoShader::draw(const Camera& camera, const Mesh& mesh, GLuint textureId) {
    const ViewPort& vp = camera.vp;
    glViewport(vp.left, vp.bottom, vp.width, vp.height);
    glBindTexture(GL_TEXTURE_2D, textureId);
    set1(u[0], 0);
    set4(u[1], camera.ortho);
    set4(u[2], mesh.modelMatrix);
    attr(a[0], mesh.position);
    attr(a[1], mesh.texture);
    drawFaces(mesh.face);
    glBindTexture(GL_TEXTURE_2D, 0);
}
