#include "geometry/Mesh.h"
#include <algorithm> // std::min, std::max

// ------------------------------------------------------------
// addQuad
// ------------------------------------------------------------
Mesh& Mesh::addQuad(float scale)
{
    vertexBuffer.clear();
    indexBuffer.clear();

    float s = scale * 0.5f;

    // Simple quad in XY plane
    vertexBuffer.emplace_back(-s, -s, 0.f, 1.f);
    vertexBuffer.emplace_back(s, -s, 0.f, 1.f);
    vertexBuffer.emplace_back(s, s, 0.f, 1.f);
    vertexBuffer.emplace_back(-s, s, 0.f, 1.f);

    indexBuffer = {
        0, 1, 2,
        2, 3, 0
    };

    computeAABB();
    return *this;
}

// ------------------------------------------------------------
// addCube
// ------------------------------------------------------------
Mesh& Mesh::addCube(float scale)
{
    vertexBuffer.clear();
    indexBuffer.clear();

    float s = scale * 0.5f;

    // 8 cube vertices
    vertexBuffer = {
        {-s, -s, -s, 1.f},
        { s, -s, -s, 1.f},
        { s,  s, -s, 1.f},
        {-s,  s, -s, 1.f},

        {-s, -s,  s, 1.f},
        { s, -s,  s, 1.f},
        { s,  s,  s, 1.f},
        {-s,  s,  s, 1.f},
    };

    // 12 triangles (36 indices)
    indexBuffer = {
        // front
        4, 5, 6,
        6, 7, 4,

        // back
        0, 1, 2,
        2, 3, 0,

        // left
        0, 4, 7,
        7, 3, 0,

        // right
        1, 5, 6,
        6, 2, 1,

        // top
        3, 2, 6,
        6, 7, 3,

        // bottom
        0, 1, 5,
        5, 4, 0
    };

    computeAABB();
    return *this;
}

// ------------------------------------------------------------
// addNet
// ------------------------------------------------------------
Mesh& Mesh::addNet(uint32_t size)
{
    vertexBuffer.clear();
    indexBuffer.clear();

    // Simple grid in XY plane
    vertexBuffer.reserve(size * size);

    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {
            vertexBuffer.emplace_back(
                static_cast<float>(x),
                static_cast<float>(y),
                0.f,
                1.f
            );
        }
    }

    // Triangulate grid
    for (uint32_t y = 0; y < size - 1; ++y) {
        for (uint32_t x = 0; x < size - 1; ++x) {
            uint32_t i0 = y * size + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + size;
            uint32_t i3 = i2 + 1;

            // two triangles
            indexBuffer.push_back(i0);
            indexBuffer.push_back(i1);
            indexBuffer.push_back(i2);

            indexBuffer.push_back(i1);
            indexBuffer.push_back(i3);
            indexBuffer.push_back(i2);
        }
    }

    computeAABB();
    return *this;
}

const std::vector<Vec4>& Mesh::getVertexBuffer() const
{
    return vertexBuffer;
}

const std::vector<uint32_t>& Mesh::getIndexBuffer() const
{
    return indexBuffer;
}

// ------------------------------------------------------------
// computeAABB
// ------------------------------------------------------------
void Mesh::computeAABB()
{
    if (vertexBuffer.empty()) {
        boundingBox = AABB{};
        return;
    }

    Vec4 minV = vertexBuffer[0];
    Vec4 maxV = vertexBuffer[0];

    for (const auto& v : vertexBuffer) {
        minV.x = std::min(minV.x, v.x);
        minV.y = std::min(minV.y, v.y);
        minV.z = std::min(minV.z, v.z);

        maxV.x = std::max(maxV.x, v.x);
        maxV.y = std::max(maxV.y, v.y);
        maxV.z = std::max(maxV.z, v.z);
    }

    boundingBox.min = minV;
    boundingBox.max = maxV;
}
