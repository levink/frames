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
    vertex.resize(vertex.size() + 4);
    face.emplace_back(i + 0, i + 1, i + 2);
    face.emplace_back(i + 2, i + 1, i + 3);
}
bool LineMesh::empty() const {
    return vertex.empty() || face.empty();
}
void LineMesh::clear() {
    vertex.clear();
    face.clear();
}
void LineMesh::createPoint(size_t vertexOffset, const glm::vec2& pos, float radius) {

    while (vertex.size() < vertexOffset + 4) {
        reserveQuad();
    }

    const auto dx = glm::vec2(radius, 0);
    const auto dy = glm::vec2(0, radius);
    vertex[0] = LineVertex{ pos - dx - dy, pos, pos, radius };
    vertex[1] = LineVertex{ pos - dx + dy, pos, pos, radius };
    vertex[2] = LineVertex{ pos + dx - dy, pos, pos, radius };
    vertex[3] = LineVertex{ pos + dx + dy, pos, pos, radius };
}

