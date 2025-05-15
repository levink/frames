#pragma once
#include "shaderBase.h"
#include "model/camera.h"
#include "model/mesh.h"

class VideoShader : public Shader {
    struct {
        const Camera* camera = nullptr;
    } context;
public:
    GLuint videoTextureId = 0;
    VideoShader();
    void link(const Camera* camera);
    void enable() const override;
    void disable() const override;
    void draw(const VideoMesh& mesh);
};