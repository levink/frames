#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <list>
#include "model/camera.h"
#include "model/mesh.h"

struct ShaderContext; //forward

struct Viewport {
    int left    = 0;   
    int bottom  = 0; 
    int width   = 1;  
    int height  = 1;
};

struct Line {
    float width;
    float radius;
    std::vector<glm::vec2> points;
    LineMesh mesh;
    explicit Line(float width);
    void addPoint(const glm::vec2& point);
};

struct FrameRender {
    Viewport vp;
	Camera cam;
    GLuint textureId = 0;
    bool textureReady = false;
	ImageMesh image;
    std::list<Line> lines;

    void create(int16_t width, int16_t height);
    void update(int16_t width, int16_t height, const uint8_t* pixels);
    void reshape(int left, int top, int width, int height, int screenHeight);
    void move(int dx, int dy);
    void zoom(float value);
    void draw(ShaderContext& shader) const;

    glm::vec2 toOpenGLSpace(int x, int y) const;
    glm::vec2 toSceneSpace(int x, int y) const;

    void newLine(int x, int y, float width);
    void addPoint(int x, int y);
};
