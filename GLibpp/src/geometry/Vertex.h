#pragma once
#include <vector>
#include "../math/Vec4.h"

class Vertex {

public:

	Vec4 pos = Vec4(0,0,0,0);
	Vec4 normal = Vec4(1,0,0,0);
	Vec4 color = Vec4(1,1,1,0);

	std::vector<Vec4> texCoords {
		Vec4(0,0,1,0)
	};

	Vertex() = default;

	Vertex(const Vec4& p) : pos(p) {}

	Vertex(float x, float y, float z) : pos(Vec4(x, y, z, 1.0f)) {}
	Vertex(float x, float y, float z, float w) : pos(Vec4(x, y, z, w)) {}


};