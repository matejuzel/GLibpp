#pragma once
#include <vector>
#include "Triangle.h"
#include "math/Mtx4.h"
#include "Material.h"
#include "AABB.h"

class Mesh {

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

	/**
	 * Vypocte struktur AABB (Axis-aligned Bounding box)
	 */
	AABB computeAABB() const;

	static float computeCameraDistanceForAABB(const AABB& box, float verticalFovRadians, float aspect);
	Mtx4 computeViewMatrixToFillScreen(float verticalFovRadians, float aspect) const;

private:

	static AABB transformAABB(const AABB& box, const Mtx4& m);
	

};

