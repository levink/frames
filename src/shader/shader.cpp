#include "shader.h"

VideoShader::VideoShader() : Shader(4, 2) {
    u[0] = Uniform("VideoTexture");
    u[1] = Uniform("Ortho");
    u[2] = Uniform("Model");
    u[3] = Uniform("View");
    a[0] = Attribute(VEC_2, "in_Position");
    a[1] = Attribute(VEC_2, "in_Texture");
}
void VideoShader::enable() const {
    Shader::enable();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, videoTextureId);
    set1(u[0], 0);
}
void VideoShader::disable() const {
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    Shader::disable();
}

void VideoShader::draw(const Camera& camera, const Mesh& mesh) {
    set4(u[1], camera.ortho);
    set4(u[2], mesh.modelMatrix);
    set4(u[3], mesh.viewMatrix);
    attr(a[0], mesh.position);
    attr(a[1], mesh.texture);
    drawFaces(mesh.face);
}
