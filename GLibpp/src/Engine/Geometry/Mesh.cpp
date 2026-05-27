#include "Mesh.h"
#include "Mathematics.h"
#include <algorithm> // std::min, std::max

/*
Mesh& Mesh::addCube(float scale)
{
    vertexBuffer.clear();
    indexBuffer.clear();

    vertexBuffer.reserve(8);
    indexBuffer.reserve(36);

    float s = scale * 0.5f;

    vertexBuffer.push_back({ -s, -s, -s, 1.f });
    vertexBuffer.push_back({ s, -s, -s, 1.f });
    vertexBuffer.push_back({ s,  s, -s, 1.f });
    vertexBuffer.push_back({ -s,  s, -s, 1.f });

    vertexBuffer.push_back({ -s, -s,  s, 1.f });
    vertexBuffer.push_back({ s, -s,  s, 1.f });
    vertexBuffer.push_back({ s,  s,  s, 1.f });
    vertexBuffer.push_back({ -s,  s,  s, 1.f });

    indexBuffer = {
        0,2,1,  2,0,3,
        4,5,6,  6,7,4,
        0,1,5,  5,4,0,
        3,6,2,  6,3,7,
        0,4,7,  7,3,0,
        1,2,6,  6,5,1
    };

    return *this;
}
*/


/*
Mesh& Mesh::addIcosan(float radius, uint32_t subdivisions)
{
    // 1) vytvoøit základní icosahedron
    addIcosahedron(radius);

    // 2) aplikovat subdivize
    for (uint32_t i = 0; i < subdivisions; i++)
        subdivide();

    // 3) normalizovat vertexy na radius
    for (auto& v : vertexBuffer) {
        float invLen = 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        v.x = (v.x * invLen) * radius;
        v.y = (v.y * invLen) * radius;
        v.z = (v.z * invLen) * radius;
    }

    return *this;
}
*/



const std::vector<Vec4>& Mesh::getVertexBuffer() const
{
    return vertexBuffer;
}

const std::vector<uint32_t>& Mesh::getIndexBuffer() const
{
    return indexBuffer;
}
/*
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
*/