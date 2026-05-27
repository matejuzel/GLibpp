#include "Mesh.h"
#include "Mathematics.h"
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

    //computeAABB();
    return *this;
}

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

Mesh& Mesh::addSphere(float radius, uint32_t segments)
{
    vertexBuffer.clear();
    indexBuffer.clear();

    if (segments < 3)
        return *this;

    const uint32_t rings = segments;
    const uint32_t sectors = segments;

    vertexBuffer.reserve((rings + 1) * (sectors + 1));
    indexBuffer.reserve(rings * sectors * 6);

    const float R = 1.0f / float(rings);
    const float S = 1.0f / float(sectors);

    for (uint32_t r = 0; r <= rings; r++)
    {
        float v = r * R;
        float phi = v * GLibpp::Math::pi;

        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (uint32_t s = 0; s <= sectors; s++)
        {
            float u = s * S;
            float theta = u * 2.0f * GLibpp::Math::pi; // 0..2PI

            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

            float x = cosTheta * sinPhi;
            float y = cosPhi;
            float z = sinTheta * sinPhi;

            vertexBuffer.emplace_back(
                x * radius,
                y * radius,
                z * radius,
                1.0f
            );
        }
    }

    // --- indexy ---
    for (uint32_t r = 0; r < rings; r++)
    {
        for (uint32_t s = 0; s < sectors; s++)
        {
            uint32_t i0 = r * (sectors + 1) + s;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (sectors + 1);
            uint32_t i3 = i2 + 1;

            // první trojúhelník
            indexBuffer.push_back(i0);
            indexBuffer.push_back(i2);
            indexBuffer.push_back(i1);

            // druhý trojúhelník
            indexBuffer.push_back(i1);
            indexBuffer.push_back(i2);
            indexBuffer.push_back(i3);
        }
    }

    return *this;
}

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

// ------------------------------------------------------------
// addNet
// ------------------------------------------------------------
Mesh& Mesh::addNet(uint32_t size, float distort)
{
    vertexBuffer.clear();
    indexBuffer.clear();

    // Simple grid in XY plane
    vertexBuffer.reserve(size * size);

    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {
            vertexBuffer.emplace_back(
                static_cast<float>(x) + distort * (static_cast<float>(rand()) / RAND_MAX - 0.5f),
                static_cast<float>(y) + distort * (static_cast<float>(rand()) / RAND_MAX - 0.5f),
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

    //computeAABB();
    return *this;
}

Mesh& Mesh::addNetWave(uint32_t size, float waveHeight, float time, float speed) {
    vertexBuffer.clear();
    indexBuffer.clear();
    vertexBuffer.reserve(size * size);

    // Støed møížky
    const float cx = (size - 1) * 0.5f;
    const float cy = (size - 1) * 0.5f;

    // Frekvence vlny (mùžeš si upravit)
    const float frequency = 1.0f;

    // --- Generování vertexù ---
    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {

            float dx = float(x) - cx;
            float dy = float(y) - cy;

            // Vzdálenost od støedu
            float dist = std::sqrt(dx * dx + dy * dy);

            // Radiální vlna
            float wave = std::sin(dist * frequency - time * speed);

            vertexBuffer.emplace_back(
                float(x),
                float(y),
                waveHeight * wave,
                1.f
            );
        }
    }

    // --- Triangulace møížky ---
    for (uint32_t y = 0; y < size - 1; ++y) {
        for (uint32_t x = 0; x < size - 1; ++x) {
            uint32_t i0 = y * size + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + size;
            uint32_t i3 = i2 + 1;

            indexBuffer.push_back(i0);
            indexBuffer.push_back(i1);
            indexBuffer.push_back(i2);

            indexBuffer.push_back(i1);
            indexBuffer.push_back(i3);
            indexBuffer.push_back(i2);
        }
    }

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