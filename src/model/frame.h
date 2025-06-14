#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "model/camera.h"
#include "model/mesh.h"

struct ViewPort {
    int x, y;   // Offset from bottom left corner of main window
    int w, h;   // View size
};

struct FrameView {
    glm::ivec2 leftTop; /* Coordinates of left-top view's corner in a window space */
    glm::ivec2 size;    //todo: size or vp.w + vp.h?
    ViewPort vp;        
	Camera cam;
	Mesh mesh;
	GLuint textureId;

    bool hit(int x, int y) const;
    glm::vec2 toOpenGLSpace(const glm::ivec2& cursor) const;
};
