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

struct Cursor {
    bool visible = false;
    glm::ivec2 screen;  //screen position
    glm::vec2 position; //render position
    LineMesh mesh;
};

struct Segment {
    glm::vec2 p1, p2;
    glm::vec2 n1, n2;
    glm::vec2 dir;
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
    Cursor cursor;
    ImageMesh imageMesh;
    
    bool draw = false;
    float lineWidth = 5.f;
    glm::vec3 backColor = { 1.f, 1.f, 1.f };
    glm::vec3 frontColor = { 1.f, 0.f, 0.f };
    std::list<Line> lines;
    LineMesh lineMesh;   

    void createTexture(int16_t width, int16_t height);
    void updateTexture(int16_t width, int16_t height, const uint8_t* pixels);
    void destroyTexture();
    void reshape(int left, int top, int width, int height, int screenHeight);
    void moveCam(int dx, int dy);
    void zoomCam(float value);
    void render(ShaderContext& shader) const;
    void setLineWidth(float width);
    void setLineColor(float r, float g, float b);
    void showCursor(bool visible);
    void drawStart(int x, int y);
    void drawNext(int x, int y, bool pressed);
    void drawStop();
    void clearDrawn();

private:
    glm::vec2 toOpenGLSpace(int x, int y) const;
    glm::vec2 toSceneSpace(int x, int y) const;
    glm::vec2 setCursor(int x, int y);
    float getLineRadius() const;
};
