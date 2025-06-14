#include "frame.h"

bool FrameView::hit(int x, int y) const {
    if (x < leftTop.x || leftTop.x + size.x < x) {
        return false;
    }
    if (y < leftTop.y || leftTop.y + size.y < y) {
        return false;
    }
    return true;
}

glm::vec2 FrameView::toOpenGLSpace(const glm::ivec2& cursor) const {
    /*
       Converting
        from window space x = [0, windowWidth], y = [0, windowHeight]   - coordinates from top left corner
        to OpenGL space x = [-1, -1], y = [1, 1]                        - coordinates from bottom left corner
    */

    // Local view coordinates from {0, 0} to {size.x, size.y} in window space
    auto local = cursor - leftTop; 
    
    // Local view coordinates from {-1, -1} to {1, 1} in OpenGL space
    auto result = glm::vec2(
        ((2.0f * local.x) / size.x) - 1.0,
        1.0 - ((2.0f * local.y) / size.y)
    );
    return result;
}