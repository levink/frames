#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <list>
#include "model/camera.h"
#include "model/mesh.h"

struct ShaderContext; //forward

struct FrameBuffer {
    GLuint fbo = 0; //frame buffer id
    GLuint rbo = 0; //render buffer id
    GLuint tid = 0; //render texture id
    int width  = 0;
    int height = 0;
    void create(float w, float h);
    void reshape(float w, float h);
    void destroy();
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
    FrameBuffer fb;
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
    void reshape(int width, int height);
    void moveCam(int dx, int dy);
    void zoomCam(float value);
    void render(ShaderContext& shader) const;
    void setBrush(const float color[3], float width);
    void showCursor(bool visible);
    void moveCursor(int x, int y);
    void drawStart(int x, int y);
    void drawNext(int x, int y, int mode); //todo: use enum for mode?
    void drawStop();
    void clearDrawn();

private:
    glm::vec2 toOpenGLSpace(int x, int y) const;
    glm::vec2 toSceneSpace(int x, int y) const;
    glm::vec2 setCursor(int x, int y);
    float getLineRadius() const;
};
