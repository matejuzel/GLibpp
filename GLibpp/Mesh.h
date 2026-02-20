#pragma once
#include <vector>
#include "Triangle.h"
#include "Mtx4.h"
#include "Material.h"
#include "AABB.h"

class Mesh {

private:

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

	Mesh& applyTransformation();

	Mesh& transform(const Mtx4 &m);

	/**
	/* Vypocte struktur AABB (Axis-aligned Bounding box)
	/*/
	AABB computeAABB() const;

};

