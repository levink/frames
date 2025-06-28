#pragma once
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

struct GLFace {
    uint16_t a = 0;
    uint16_t b = 0;
    uint16_t c = 0;
};

struct Mesh {
    std::vector<glm::vec2> position;
    std::vector<glm::vec2> texture;
    std::vector<GLFace> face;

    static Mesh createImageMesh(int w, int h);
};

struct CircleMesh {
    std::vector<glm::vec2> position;
    std::vector<glm::vec2> center;
    std::vector<float> radius;
    std::vector<GLFace> face;

    CircleMesh();
    void add(int x, int y, float radius);
};
