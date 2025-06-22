#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "model/camera.h"
#include "model/mesh.h"


struct Frame {
    glm::ivec2 viewPos;     // Offset from bottom left corner of main window for using in glViewport(...)
    glm::ivec2 viewSize;    // View size in pixels in a window space
    
	Camera cam;
	Mesh mesh;
	GLuint textureId = 0;

    void reshape(int left, int top, int width, int height, int screenHeight);
    glm::vec2 toOpenGLSpace(const glm::ivec2& cursor) const;    //todo: check code twice + check comments
    glm::vec2 toSceneSpace(const glm::ivec2& cursor) const;     //todo: check code twice + check comments
};
