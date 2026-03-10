#pragma once
#include <vector>
#include "Triangle.h"
#include "math/Mtx4.h"
#include "Material.h"
#include "AABB.h"
#include "math/Mtx4.h"
#include "utils/datastruct/AssetRegistry.h"

using mesh_handle_t = asset_handle_t;
using material_handle_t = asset_handle_t;

class Mesh {
public:

	Mesh() = default;

	static std::unique_ptr<Mesh> Cube(float scale) {
		auto m = std::make_unique<Mesh>();
		m->addCube(scale);
		return m;
	}

	static std::unique_ptr<Mesh> Net(uint32_t size) {
		auto m = std::make_unique<Mesh>();
		m->addNet(size);
		return m;
	}

	Mesh& addQuad(float scale);
	Mesh& addCube(float scale);
	Mesh& addNet(uint32_t size);

	const std::vector<Vec4>& getVertexBuffer() const;

	const std::vector<uint32_t>& getIndexBuffer() const;

private:

	void computeAABB();


	std::vector<Vec4> vertexBuffer;
	std::vector<uint32_t> indexBuffer;
	AABB boundingBox;
};

