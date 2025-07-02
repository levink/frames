#pragma once
#include "shaderBase.h"
#include "model/frame.h"

class VideoShader : public Shader {
public:
    VideoShader();
    void enable() const override;
    void disable() const override;
    void render(const FrameRender& frame);
};

class LinesShader : public Shader {
public:
    LinesShader();
    void enable() const override;
    void disable() const override;
    void render(const FrameRender& frame);
};

class PointShader : public Shader {
public:
    PointShader();
    void enable() const override;
    void disable() const override;
    void render(const Camera& cam, const std::vector<glm::vec2>& points);
};

struct ShaderContext {
    VideoShader video;
    LinesShader lines;
    PointShader point;
};