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

namespace {
    constexpr size_t LINE_SIZE_MAX = 4 * 100;
    constexpr size_t LINE_FACE_MAX = 2 * 100;
}

LineMesh::LineMesh() {
    vertex.reserve(LINE_SIZE_MAX);
    face.reserve(LINE_FACE_MAX);
}
void LineMesh::addPoint(int x, int y, float r) {
    if (vertex.size() >= LINE_SIZE_MAX) {
        return;
    }

    vec2 p0 = { x, y };
    vec2 p1 = { x, y };
    vertex.emplace_back(LineVertex{ { x - r, y - r }, p0, p1, r });
    vertex.emplace_back(LineVertex{ { x - r, y + r }, p0, p1, r });
    vertex.emplace_back(LineVertex{ { x + r, y + r }, p0, p1, r });
    vertex.emplace_back(LineVertex{ { x + r, y - r }, p0, p1, r });

    const uint16_t i0 = vertex.size() - 4;
    const uint16_t i1 = i0 + 1;
    const uint16_t i2 = i0 + 2;
    const uint16_t i3 = i0 + 3;
    face.emplace_back(i0, i1, i2);
    face.emplace_back(i2, i3, i0);
}

