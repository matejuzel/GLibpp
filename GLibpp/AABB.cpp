#include "AABB.h"

std::string AABB::toString() const {
	std::ostringstream ss;
	ss << "AABB[" << std::endl << "Min: " << min.toString() << std::endl << "Max: " << max.toString() << std::endl << "==============";
	return ss.str();
}
