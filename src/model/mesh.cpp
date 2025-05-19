#include "mesh.h"
#include <glm/gtc/matrix_transform.hpp>

void VideoMesh::setSize(int w, int h) {
    position = {
        { 0, 0 },
        { w, 0 },
        { w, h },
        { 0, h }
    };
    texture = {
        {0, 1},
        {1, 1},
        {1, 0},
        {0, 0}
    };
    face = {
        { 0, 1, 2 },
        { 2, 3, 0 }
    };
}

void VideoMesh::move(int dx, int dy) {
    modelMatrix = glm::translate(modelMatrix, { dx, dy, 0 });
}

void VideoMesh::scale(float value) {
    modelMatrix = glm::scale(modelMatrix, { value, value, 1 });
}

