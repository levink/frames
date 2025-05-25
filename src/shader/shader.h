#pragma once
#include "shaderBase.h"
#include "model/camera.h"
#include "model/mesh.h"

class VideoShader : public Shader {
public:
    VideoShader();
    void enable() const override;
    void disable() const override;
    void draw(const Camera& camera, const Mesh& mesh, GLuint textureId);
};