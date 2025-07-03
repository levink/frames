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
    //glEnable(GL_DEPTH_TEST);
}
void VideoShader::disable() const {
    Shader::disable();
    glDisable(GL_TEXTURE_2D);
    //glDisable(GL_DEPTH_TEST);
}
void VideoShader::render(const FrameRender& frame) {
    if (!frame.textureReady) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, frame.textureId);
    set1(u[0], 0);
    set4(u[1], frame.cam.proj);
    set4(u[2], frame.cam.view);
    attr(a[0], frame.imageMesh.position);
    attr(a[1], frame.imageMesh.texture);
    drawFaces(frame.imageMesh.face);
    glBindTexture(GL_TEXTURE_2D, 0);
}

LinesShader::LinesShader() : Shader(5, 4) {
    u[0] = Uniform("Proj");
    u[1] = Uniform("View");
    u[2] = Uniform("Color");
    u[3] = Uniform("Offset");
    u[4] = Uniform("Scale");

    a[0] = Attribute(VEC_2, "in_Position");
    a[1] = Attribute(VEC_2, "in_LineStart");
    a[2] = Attribute(VEC_2, "in_LineEnd");
    a[3] = Attribute(VEC_2, "in_Radius");
}
void LinesShader::enable() const {
    Shader::enable();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void LinesShader::disable() const {
    Shader::disable();
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}
void LinesShader::render(const FrameRender& frame) {
    
    set4(u[0], frame.cam.proj);
    set4(u[1], frame.cam.view);
    set1(u[4], frame.cam.scale_inverse);

    static const auto whiteColor = glm::vec3(1.f, 1.f, 1.f);

    if (!frame.lineMesh.empty()) {
        const auto& mesh = frame.lineMesh;
        const auto& vertex = mesh.vertex.data();
        attr(a[0], vertex, sizeof(LineVertex), offsetof(LineVertex, position));
        attr(a[1], vertex, sizeof(LineVertex), offsetof(LineVertex, segmentP0));
        attr(a[2], vertex, sizeof(LineVertex), offsetof(LineVertex, segmentP1));
        attr(a[3], vertex, sizeof(LineVertex), offsetof(LineVertex, radius));

        //background
        set3(u[2], whiteColor);
        set1(u[3], 0.f);
        drawFaces(mesh.face);

        //foreground
        set3(u[2], mesh.color);
        set1(u[3], -1.5f);
        drawFaces(mesh.face);
    }

    if (frame.cursor.visible && !frame.cursor.mesh.empty()) {
        const auto& mesh = frame.cursor.mesh;
        const auto& vertex = mesh.vertex.data();
        attr(a[0], vertex, sizeof(LineVertex), offsetof(LineVertex, position));
        attr(a[1], vertex, sizeof(LineVertex), offsetof(LineVertex, segmentP0));
        attr(a[2], vertex, sizeof(LineVertex), offsetof(LineVertex, segmentP1));
        attr(a[3], vertex, sizeof(LineVertex), offsetof(LineVertex, radius));

        //background
        set3(u[2], whiteColor);
        set1(u[3], 0.f);
        drawFaces(mesh.face);

        //foreground
        set3(u[2], mesh.color);
        set1(u[3], -1.5f);
        drawFaces(mesh.face);
    }


}

PointShader::PointShader() : Shader(2, 1) {
    u[0] = Uniform("Proj");
    u[1] = Uniform("View");
    a[0] = Attribute(VEC_2, "in_Position");
}
void PointShader::enable() const {
    Shader::enable();
    glDisable(GL_DEPTH_TEST);
    //glDepthMask(GL_FALSE);
    glEnable(0x8642); //GL_PROGRAM_POINT_SIZE
}
void PointShader::disable() const {
    Shader::disable();
    //glDepthMask(GL_TRUE);
    glDisable(0x8642); //GL_PROGRAM_POINT_SIZE
}
void PointShader::render(const Camera& cam, const std::vector<glm::vec2>& points) {
    if (points.empty()) {
        return;
    }
    set4(u[0], cam.proj);
    set4(u[1], cam.view);
    attr(a[0], points.data(), 0, 0);
    glDrawArrays(GL_LINE_STRIP, 0, points.size());
    glDrawArrays(GL_POINTS, 0, points.size());
}
