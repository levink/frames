#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "model/camera.h"
#include "model/mesh.h"


struct FrameView {
    glm::ivec2 leftTop;  // Coordinates of left-top view's corner in a window space
    glm::ivec2 viewPort; // Viewport offset from bottom left corner of main window
    glm::ivec2 viewSize; // View size in pixels in a window space
    
	Camera cam;
	Mesh mesh;
	GLuint textureId = 0;

    bool hit(int x, int y) const;
    glm::vec2 toOpenGLSpace(const glm::ivec2& cursor) const;
    glm::vec2 toSceneSpace(const glm::ivec2& cursor) const;
};
