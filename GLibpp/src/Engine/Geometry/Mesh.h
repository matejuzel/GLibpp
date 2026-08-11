#pragma once
#include <vector>
#include "Mtx4.h"

// loadery modelu (GLibpp::Assets) plni Mesh vyhradne pres MeshAccess
namespace GLibpp::Assets { struct MeshAccess; }

namespace GLibpp::Geometry {

class Mesh {

	friend class MeshFactory;
    friend class MeshModifier;
    friend struct GLibpp::Assets::MeshAccess;

public:

	Mesh() = default;

    Mesh& applyTransformation(const Mtx4& mtx)
    {
        for (auto& v : vertexBuffer) {
            v = mtx * v;
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

	const std::vector<float>& getUVBuffer() const;

private:

	//void computeAABB();


	std::vector<Vec4> vertexBuffer;
	std::vector<uint32_t> indexBuffer;

	// texturovaci UV per vrchol, prokladane [u0, v0, u1, v1, ...];
	// prazdny buffer = mesh bez UV (jinak size == 2 * vertexBuffer.size()).
	// Pozn.: MeshModifier::Subdivide UV nedopocitava - pouziva se jen na
	// icosphere, ktera UV nema
	std::vector<float> uvBuffer;
	//AABB boundingBox;
};

}

