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

struct Segment {
    glm::vec2 p1, p2;
    glm::vec2 n1, n2;
    glm::vec2 dir;
    bool openLeft = true;
    bool openRight = true;
    Segment() = default;
    Segment(const glm::vec2& p1, const glm::vec2& p2);
    void moveP2(const glm::vec2& position);
};

struct Line {
    float radius;
    std::vector<glm::vec2> points;
    std::vector<Segment> segments;
    size_t meshOffset;

    Line(float radius, size_t meshOffset);
    void addPoint(const glm::vec2& pos, LineMesh& mesh);
    void moveLast(const glm::vec2& pos);
    void updateMesh(LineMesh& mesh);
};

struct FrameRender {
    Viewport vp;
	Camera cam;
    
    GLuint textureId = 0;       //todo: move to imageMesh?
    bool textureReady = false;  //todo: move to imageMesh?
	ImageMesh imageMesh;
    
    float lineWidth = 10.f;
    glm::vec3 backColor = { 1.f, 1.f, 1.f };
    glm::vec3 frontColor = { 1.f, 0.f, 0.f };
    std::list<Line> lines;
    LineMesh lineMesh;

    void create(int16_t width, int16_t height);
    void update(int16_t width, int16_t height, const uint8_t* pixels);
    void reshape(int left, int top, int width, int height, int screenHeight);
    void move(int dx, int dy);
    void zoom(float value);
    void render(ShaderContext& shader) const;
    void setLineWidth(float width);
    void setLineColor(float r, float g, float b);
    void mouseClick(int x, int y);
    void mouseDrag(int x, int y);
    void mouseStop(int x, int y);
    void clearDrawn();

private:
    glm::vec2 toOpenGLSpace(int x, int y) const;
    glm::vec2 toSceneSpace(int x, int y) const;
};
