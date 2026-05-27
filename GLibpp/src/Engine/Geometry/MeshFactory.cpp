
#include "Mesh.h"
#include "MeshFactory.h"
#include "MeshModifier.h"
#include "Mathematics.h"

Mesh MeshFactory::CreateCube(float side) {
	
    Mesh msh;

    msh.vertexBuffer.clear();
    msh.indexBuffer.clear();

    msh.vertexBuffer.reserve(8);
    msh.indexBuffer.reserve(36);
    float s = side * 0.5f;

    msh.vertexBuffer.push_back({ -s, -s, -s, 1.f });
    msh.vertexBuffer.push_back({ s, -s, -s, 1.f });
    msh.vertexBuffer.push_back({ s,  s, -s, 1.f });
    msh.vertexBuffer.push_back({ -s,  s, -s, 1.f });

    msh.vertexBuffer.push_back({ -s, -s,  s, 1.f });
    msh.vertexBuffer.push_back({ s, -s,  s, 1.f });
    msh.vertexBuffer.push_back({ s,  s,  s, 1.f });
    msh.vertexBuffer.push_back({ -s,  s,  s, 1.f });

    msh.indexBuffer = {
        0,2,1,  2,0,3,
        4,5,6,  6,7,4,
        0,1,5,  5,4,0,
        3,6,2,  6,3,7,
        0,4,7,  7,3,0,
        1,2,6,  6,5,1
    };

    return msh;
}

Mesh MeshFactory::CreateSphere(float radius, uint32_t segments) 
{
    Mesh msh;
    msh.vertexBuffer.clear();
    msh.indexBuffer.clear();

    if (segments < 3) return msh;

    const uint32_t rings = segments;
    const uint32_t sectors = segments;

    msh.vertexBuffer.reserve((rings + 1) * (sectors + 1));
    msh.indexBuffer.reserve(rings * sectors * 6);
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

            msh.vertexBuffer.emplace_back(
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
            msh.indexBuffer.push_back(i0);
            msh.indexBuffer.push_back(i2);
            msh.indexBuffer.push_back(i1);

            // druhý trojúhelník
            msh.indexBuffer.push_back(i1);
            msh.indexBuffer.push_back(i2);
            msh.indexBuffer.push_back(i3);
        }
    }
}

Mesh MeshFactory::CreateCylinder(float radius, float height, uint32_t segments) { throw std::runtime_error("Not implemented"); }


Mesh MeshFactory::CreateIcosan(float radius)
{
    Mesh msh;

    msh.vertexBuffer.clear();
    msh.indexBuffer.clear();

    const float t = (1.0f + sqrtf(5.0f)) * 0.5f;

    // 12 vertexù icosahedronu
    msh.vertexBuffer = {
        { -1.0f,  t,  0.0f, 1.0f },{ 1.0f,  t,  0.0f, 1.0f },{ -1.0f, -t,  0.0f, 1.0f },{ 1.0f, -t,  0.0f, 1.0f },
        { 0.0f, -1.0f,  t, 1.0f },{ 0.0f,  1.0f,  t, 1.0f },{ 0.0f, -1.0f, -t, 1.0f },{ 0.0f,  1.0f, -t, 1.0f },
        { t,  0.0f, -1.0f, 1.0f },{ t,  0.0f,  1.0f, 1.0f },{ -t,  0.0f, -1.0f, 1.0f },{ -t,  0.0f,  1.0f, 1.0f }
    };

    // normalizace na radius
    for (auto& v : msh.vertexBuffer) {
        float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        v.x = (v.x / len) * radius;
        v.y = (v.y / len) * radius;
        v.z = (v.z / len) * radius;
    }

    // 20 trojúhelníkù
    msh.indexBuffer = {
        0,11,5,  0,5,1,  0,1,7,  0,7,10, 0,10,11,
        1,5,9,   5,11,4, 11,10,2, 10,7,6, 7,1,8,
        3,9,4,   3,4,2,  3,2,6,  3,6,8,  3,8,9,
        4,9,5,   2,4,11, 6,2,10, 8,6,7,  9,8,1
    };

    return msh;
}

Mesh MeshFactory::CreateIcosphere(float radius, uint32_t subdivisions) 
{
    Mesh msh = MeshFactory::CreateIcosan(radius);

    for (uint32_t i = 0; i < subdivisions; i++) 
    {
        MeshModifier::Subdivide(msh);
    }
    
    // 3) normalizovat vertexy na radius
    for (auto& v : msh.vertexBuffer) {
        float invLen = 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        v.x = (v.x * invLen) * radius;
        v.y = (v.y * invLen) * radius;
        v.z = (v.z * invLen) * radius;
    }

    return msh;
}

Mesh MeshFactory::CreateNet(uint32_t size, float distort) 
{
    Mesh msh;

    msh.vertexBuffer.clear();
    msh.indexBuffer.clear();

    // Simple grid in XY plane
    msh.vertexBuffer.reserve(size * size);

    for (uint32_t y = 0; y < size; ++y) {
        for (uint32_t x = 0; x < size; ++x) {
            msh.vertexBuffer.emplace_back(
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
            msh.indexBuffer.push_back(i0);
            msh.indexBuffer.push_back(i1);
            msh.indexBuffer.push_back(i2);

            msh.indexBuffer.push_back(i1);
            msh.indexBuffer.push_back(i3);
            msh.indexBuffer.push_back(i2);
        }
    }

    return msh;
}

Mesh MeshFactory::CreateNetWave(uint32_t size, float waveHeight, float time, float speed) 
{
    Mesh msh;

    msh.vertexBuffer.clear();
    msh.indexBuffer.clear();
    msh.vertexBuffer.reserve(size * size);

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

            msh.vertexBuffer.emplace_back(
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

            msh.indexBuffer.push_back(i0);
            msh.indexBuffer.push_back(i1);
            msh.indexBuffer.push_back(i2);

            msh.indexBuffer.push_back(i1);
            msh.indexBuffer.push_back(i3);
            msh.indexBuffer.push_back(i2);
        }
    }
    return msh;
}

Mesh MeshFactory::CreateQuad(float size) 
{
    Mesh msh;
    msh.vertexBuffer.clear();
    msh.indexBuffer.clear();

    float s = size * 0.5f;

    // Simple quad in XY plane
    msh.vertexBuffer.emplace_back(-s, -s, 0.f, 1.f);
    msh.vertexBuffer.emplace_back(s, -s, 0.f, 1.f);
    msh.vertexBuffer.emplace_back(s, s, 0.f, 1.f);
    msh.vertexBuffer.emplace_back(-s, s, 0.f, 1.f);

    msh.indexBuffer = {
        0, 1, 2,
        2, 3, 0
    };
    return msh;
}

