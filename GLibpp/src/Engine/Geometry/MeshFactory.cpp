
#include "Mesh.h"
#include "MeshFactory.h"
#include "MeshModifier.h"

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

Mesh MeshFactory::CreateSphere(float radius) { throw std::runtime_error("Not implemented"); }

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

Mesh MeshFactory::CreateNet(uint32_t size, float distort) { throw std::runtime_error("Not implemented"); }

Mesh MeshFactory::CreateNetWave(uint32_t size, float waveHeight, float time, float speed) { throw std::runtime_error("Not implemented"); }

Mesh MeshFactory::CreateQuad(float size) { throw std::runtime_error("Not implemented"); }

