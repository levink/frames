#include "mesh.h"

using std::vector;
using glm::vec2;

ImageMesh ImageMesh::createImageMesh(int w, int h) {
    auto position = vector<vec2> {
        { 0, 0 },
        { w, 0 },
        { w, h },
        { 0, h }
    };
    auto texture = vector<vec2> {
        { 0, 1 },
        { 1, 1 },
        { 1, 0 },
        { 0, 0 }
    };
    auto face = vector<GLFace>{
       { 0, 1, 2 },
       { 2, 3, 0 }
    };
    return ImageMesh{
        std::move(position),
        std::move(texture),
        std::move(face)
    };
}

void LineMesh::reserveQuad() {
    const uint16_t i = vertex.size();
    vertex.emplace_back();
    vertex.emplace_back();
    vertex.emplace_back();
    vertex.emplace_back();
    face.emplace_back(i + 0, i + 1, i + 2);
    face.emplace_back(i + 2, i + 1, i + 3);
}
bool LineMesh::empty() const {
    return vertex.empty() || face.empty();
}

