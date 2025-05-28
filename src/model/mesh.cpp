#include "mesh.h"
#include <glm/gtc/matrix_transform.hpp>

void Mesh::setSize(int w, int h) {
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

    offset = { -w/2, -h/2, 0 };
    scale = { 1, 1, 1 };
    updateMatrix();
}

void Mesh::updateMatrix() {
    modelMatrix = glm::scale(glm::mat4(1), scale);
    modelMatrix = glm::translate(modelMatrix, offset);
}

void Mesh::move(int deltaX, int deltaY) {
    offset.x += deltaX / scale.x;
    offset.y += deltaY / scale.y;
    updateMatrix();
}

void Mesh::zoom(float value) {
    scale.x += value;
    scale.y += value;
    updateMatrix();
}

