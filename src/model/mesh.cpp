#include "mesh.h"

using std::vector;
using glm::vec2;

Mesh Mesh::createImageMesh(int w, int h) {
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
    return Mesh{
        std::move(position),
        std::move(texture),
        std::move(face)
    };
}

CircleMesh::CircleMesh() {
    position.reserve(UINT16_MAX);
    center.reserve(UINT16_MAX);
    radius.reserve(UINT16_MAX);
    face.reserve(size_t(UINT16_MAX / 2) + 2);
}
void CircleMesh::add(int x, int y, float r) {
    if (position.size() + 4 >= UINT16_MAX) {
        return;
    }

    position.emplace_back(x - r, y - r);
    position.emplace_back(x - r, y + r);
    position.emplace_back(x + r, y + r);
    position.emplace_back(x + r, y - r);

    center.emplace_back(x, y);
    center.emplace_back(x, y);
    center.emplace_back(x, y);
    center.emplace_back(x, y);

    radius.emplace_back(r);
    radius.emplace_back(r);
    radius.emplace_back(r);
    radius.emplace_back(r);

    const uint16_t i0 = position.size() - 4;
    const uint16_t i1 = i0 + 1;
    const uint16_t i2 = i0 + 2;
    const uint16_t i3 = i0 + 3;
    face.emplace_back(i0, i1, i2);
    face.emplace_back(i2, i3, i0);
}

