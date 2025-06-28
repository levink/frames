#pragma once
#include "shaderBase.h"
#include "model/frame.h"

class VideoShader : public Shader {
public:
    VideoShader();
    void enable() const override;
    void disable() const override;
    void draw(const FrameRender& frame);
};

class CircleShader : public Shader{
public:
    CircleShader();
    void enable() const override;
    void disable() const override;
    void draw(const FrameRender& frame);
};