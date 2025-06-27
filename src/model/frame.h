#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "model/camera.h"
#include "model/mesh.h"

struct GLViewport {
    int left    = 0;   
    int bottom  = 0; 
    int width   = 1;  
    int height  = 1;
};

struct Frame {
    GLViewport vp;
	Camera cam;
	Mesh mesh;
	GLuint textureId = 0;

    void create(int16_t width, int16_t height);
    void update(int16_t width, int16_t height, const uint8_t* pixels);
    void reshape(int left, int top, int width, int height, int screenHeight);
    glm::vec2 toOpenGLSpace(const glm::ivec2& cursor) const;    //todo: check code twice + check comments
    glm::vec2 toSceneSpace(const glm::ivec2& cursor) const;     //todo: check code twice + check comments
};
