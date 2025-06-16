#include "mesh.h"

Mesh Mesh::createImageMesh(int w, int h) {
    auto position = std::vector<glm::vec2> {
        { 0, 0 },
        { w, 0 },
        { w, h },
        { 0, h }
    };
    auto texture = std::vector<glm::vec2> {
        { 0, 1 },
        { 1, 1 },
        { 1, 0 },
        { 0, 0 }
    };
    auto face = std::vector<GLFace>{
       { 0, 1, 2 },
       { 2, 3, 0 }
    };
    return Mesh{
        std::move(position),
        std::move(texture),
        std::move(face)
    };
}