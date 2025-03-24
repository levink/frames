#include "shader.h"

VideoShader::VideoShader() : Shader(1, 1) {
    u[0] = Uniform("Ortho");
    a[0] = Attribute(VEC_2, "in_Position");
    //a[1] = Attribute(VEC_2, "in_Texture");
}
void VideoShader::link(const Camera* camera) {
    context.camera = camera;
}
void VideoShader::enable() const {
    Shader::enable();
    glDisable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT_AND_BACK);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}
void VideoShader::disable() const {
    Shader::disable();
}
void VideoShader::draw(const VideoMesh& mesh) {
    set4(u[0], context.camera->Ortho);
    attr(a[0], mesh.position.data(), 0, 0);
    drawFaces(mesh.face);
}
