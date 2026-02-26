#pragma once
#include <cstring>
#include "math/Vec4.h"
#include "math/Mtx4.h"

class AABB
{
public:
	Vec4 min;
	Vec4 max;

	std::string toString() const;

	// transform this AABB by matrix m and return resulting AABB in world space
	AABB transform(const Mtx4& m) const;

	Vec4 center() const;
	Vec4 extents() const;

};

