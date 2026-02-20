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

};

