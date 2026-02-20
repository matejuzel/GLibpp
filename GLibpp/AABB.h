#pragma once
#include <cstring>
#include "Vec4.h"

class AABB
{
public:
	Vec4 min;
	Vec4 max;


	std::string toString() const;

};

