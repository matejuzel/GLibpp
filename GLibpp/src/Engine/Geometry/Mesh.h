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

private:

	//void computeAABB();


	std::vector<Vec4> vertexBuffer;
	std::vector<uint32_t> indexBuffer;
	//AABB boundingBox;
};

