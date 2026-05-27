#pragma once
#include <vector>
#include "Mtx4.h"
#include "Material.h"
#include <unordered_map>

class Mesh {

	friend class MeshFactory;
    friend class MeshModifier;

public:

	Mesh() = default;


    static Mesh Cylinder(float radius, float height, uint32_t segments) {
        Mesh msh;
        msh.addCylinder(radius, height, segments);
        return msh;
    }

    Mesh& applyTransformation(const Mtx4& mtx)
    {
        for (auto& v : vertexBuffer) {
            v = mtx * v;
        }
        return *this;
    }

    Mesh& addCylinder(float radius, float height, uint32_t segments) {

        const float halfH = height * 0.5f;
        const uint32_t baseIndex = static_cast<uint32_t>(vertexBuffer.size());

        // --- Top + bottom center vertices ---
        uint32_t topCenter = baseIndex + 0;
        uint32_t bottomCenter = baseIndex + 1;

        vertexBuffer.emplace_back( 0.0f,  halfH, 0.0f, 1.0f);
        vertexBuffer.emplace_back( 0.0f, -halfH, 0.0f, 1.0f);

        // --- Ring vertices ---
        for (uint32_t i = 0; i < segments; i++) {
            float angle = (float)i / segments * 2.0f * 3.1415926535f;
            float x = std::cos(angle) * radius;
            float z = std::sin(angle) * radius;

            // top ring
            vertexBuffer.emplace_back( x,  halfH, z, 1.0f );
            // bottom ring
            vertexBuffer.emplace_back( x, -halfH, z, 1.0f);
        }

        // --- Indices ---
        // Top cap
        for (uint32_t i = 0; i < segments; i++) {
            uint32_t a = topCenter;
            uint32_t b = baseIndex + 2 + (i * 2);
            uint32_t c = baseIndex + 2 + ((i * 2 + 2) % (segments * 2));
            indexBuffer.push_back(a);
            indexBuffer.push_back(b);
            indexBuffer.push_back(c);
        }

        // Bottom cap
        for (uint32_t i = 0; i < segments; i++) {
            uint32_t a = bottomCenter;
            uint32_t b = baseIndex + 3 + ((i * 2 + 2) % (segments * 2));
            uint32_t c = baseIndex + 3 + (i * 2);
            indexBuffer.push_back(a);
            indexBuffer.push_back(b);
            indexBuffer.push_back(c);
        }

        // Side walls
        for (uint32_t i = 0; i < segments; i++) {
            uint32_t topA = baseIndex + 2 + (i * 2);
            uint32_t botA = baseIndex + 3 + (i * 2);
            uint32_t topB = baseIndex + 2 + ((i * 2 + 2) % (segments * 2));
            uint32_t botB = baseIndex + 3 + ((i * 2 + 2) % (segments * 2));

            // quad = two triangles
            indexBuffer.push_back(topA);
            indexBuffer.push_back(botA);
            indexBuffer.push_back(topB);

            indexBuffer.push_back(topB);
            indexBuffer.push_back(botA);
            indexBuffer.push_back(botB);
        }

        return *this;
    }

    Mesh& flipFaces()
    {
        // každý trojúhelník má 3 indexy
        for (size_t i = 0; i + 2 < indexBuffer.size(); i += 3)
        {
            std::swap(indexBuffer[i + 1], indexBuffer[i + 2]);
        }
        return *this;
    }


	const std::vector<Vec4>& getVertexBuffer() const;

	const std::vector<uint32_t>& getIndexBuffer() const;

private:

	//void computeAABB();


	std::vector<Vec4> vertexBuffer;
	std::vector<uint32_t> indexBuffer;
	//AABB boundingBox;
};

