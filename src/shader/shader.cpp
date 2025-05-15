#include "shader.h"

VideoShader::VideoShader() : Shader(2, 2) {
    u[0] = Uniform("VideoTexture");
    u[1] = Uniform("Ortho");
    a[0] = Attribute(VEC_2, "in_Position");
    a[1] = Attribute(VEC_2, "in_Texture");
}
void VideoShader::link(const Camera* camera) {
    context.camera = camera;
}
void VideoShader::enable() const {
    Shader::enable();
    glEnable(GL_TEXTURE_2D);
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, videoTextureId);
    set1(u[0], 0);
    //glDisable(GL_DEPTH_TEST);
    //glDisable(GL_CULL_FACE);
    //glCullFace(GL_FRONT_AND_BACK);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}
void VideoShader::disable() const {
    Shader::disable();
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
void VideoShader::draw(const VideoMesh& mesh) {
    set4(u[1], context.camera->Ortho);
    attr(a[0], mesh.position);
    attr(a[1], mesh.texture);
    drawFaces(mesh.face);
}
