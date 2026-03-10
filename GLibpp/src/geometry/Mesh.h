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

	Mesh& addCube(float scale);
	Mesh& addNet(uint32_t size);

private:
	std::vector<Vec4> vertexBuffer;
	std::vector <uint32_t> indexBuffer;
	AABB boundingBox;
};

class Color {
public:
	Color(uint32_t raw) : raw(raw) {}
	uint32_t raw;
};

class Material {
public:

	static std::unique_ptr<Material> Create(Color color) {
		auto mat = std::make_unique<Material>();
		mat->color = color;
		return mat;
	}

	Material() = default;

	Color color;
	// Pozdìji...:
	// - shader handle
	// - textury
	// - uniformy
	// - barvy
	// - parametry
	// - pipeline state
};





/*
class Mesh__ {

public:

	std::vector<Triangle> tris;
	Mtx4 transformation{ Mtx4::identity() };
	Material material;

public:
	Mesh() = default;

	Mesh& addTriangle(const Triangle& triangle);
	Mesh& addTriangle(const Vertex& a, const Vertex& b, const Vertex& c);
	Mesh& addTriangle(
				float ax, float ay, float az, 
				float bx, float by, float bz, 
				float cx, float cy, float cz);

	Mesh& addQuad(const Vertex& a, const Vertex& b, const Vertex& c, const Vertex& d);

	Mesh& addCube(float scaleFactor);

	Mesh& addNet(int divisions);

	Mesh& applyTransformation();

	Mesh& transform(const Mtx4 &m);

	AABB computeAABB() const;

	static float computeCameraDistanceForAABB(const AABB& box, float verticalFovRadians, float aspect);
	Mtx4 computeViewMatrixToFillScreen(float verticalFovRadians, float aspect) const;

private:

	static AABB transformAABB(const AABB& box, const Mtx4& m);
	

};
*/
