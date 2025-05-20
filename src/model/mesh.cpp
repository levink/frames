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

    size = { w, h };
    offset = { 0, 0 };// { -w / 2.f, -h / 2.f };
    scale = { 1, 1 };
    modelMatrix = glm::translate(glm::mat4(1), { offset.x, offset.y, 0 });
}

void Mesh::move(int x, int y) {
    offset.x = x;
    offset.y = y;
    modelMatrix = glm::translate(glm::mat4(1), { offset.x, offset.y, 0 });
}

void Mesh::zoom(float value) {
    scale += value;

   /* glm::vec3 point = {
        zoomPoint.x,
        zoomPoint.y,
        0.f
    };
    
    viewMatrix = glm::translate(glm::mat4(1), point);
    viewMatrix = glm::scale(viewMatrix, { scale.x, scale.y, 1 });
    viewMatrix = glm::translate(viewMatrix, -point);*/
}

