#pragma once
#include <vector>
#include "Mtx4.h"
#include "Material.h"

class Mesh {
public:

	Mesh() = default;

	static Mesh Cube(float scale) {
		Mesh msh;
		msh.addCube(scale);
		return msh;
	}

	static Mesh Net(uint32_t size) {
		Mesh msh;
		msh.addNet(size);
		return msh;
	}

	Mesh& addQuad(float scale);
	Mesh& addCube(float scale);
	Mesh& addNet(uint32_t size);

	const std::vector<Vec4>& getVertexBuffer() const;

	const std::vector<uint32_t>& getIndexBuffer() const;

private:

	//void computeAABB();


	std::vector<Vec4> vertexBuffer;
	std::vector<uint32_t> indexBuffer;
	//AABB boundingBox;
};

