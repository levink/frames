#include "frame.h"

void Frame::reshape(int left, int top, int width, int height, int screenHeight) {
    leftTop = { left, top };
    viewPort = { left, screenHeight - (top + height) };
    viewSize = { width, height };
    cam.reshape(width, height);
}

bool Frame::hit(int x, int y) const {
    if (x < leftTop.x || leftTop.x + viewSize.x < x) {
        return false;
    }
    if (y < leftTop.y || leftTop.y + viewSize.y < y) {
        return false;
    }
    return true;
}

glm::vec2 Frame::toOpenGLSpace(const glm::ivec2& cursor) const {
    /*
       Converting
       from window space x = [0, windowWidth], y = [0, windowHeight]   - coordinates from top left corner of window(!)
       to OpenGL space x = [-1, -1], y = [1, 1]                        - coordinates from bottom left corner of view(!)
    */

    // Coordinates from [0, 0] to [viewSize.x, viewSize.y] in window space
    auto viewCoords = cursor - leftTop; 
    
    // Coordinates from [-1, -1] to [1, 1] in OpenGL space
    auto result = glm::vec2(
        ((2.0f * viewCoords.x) / viewSize.x) - 1.0,
        1.0 - ((2.0f * viewCoords.y) / viewSize.y)
    );
    return result;
}

glm::vec2 Frame::toSceneSpace(const glm::ivec2& cursor) const {
    auto point2D = toOpenGLSpace(cursor); // x=[-1, 1], y=[-1, 1]
    auto point4D = cam.pv_inverse * glm::vec4(point2D.x, point2D.y, 0.5f, 1.f);
    return glm::vec2(point4D) / point4D.w;
}