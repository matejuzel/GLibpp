#include "AABB.h"

#include <algorithm>


std::string AABB::toString() const {
	std::ostringstream ss;
	ss << "AABB[" << std::endl << "Min: " << min.toString() << std::endl << "Max: " << max.toString() << std::endl << "==============";
	return ss.str();
}


AABB AABB::transform(const Mtx4& m) const {
    AABB out;
    Vec4 corners[8] = {
        { min.x, min.y, min.z, 1 },
        { min.x, min.y, max.z, 1 },
        { min.x, max.y, min.z, 1 },
        { min.x, max.y, max.z, 1 },
        { max.x, min.y, min.z, 1 },
        { max.x, min.y, max.z, 1 },
        { max.x, max.y, min.z, 1 },
        { max.x, max.y, max.z, 1 }
    };

    out.min = Vec4(+INFINITY, +INFINITY, +INFINITY, 1);
    out.max = Vec4(-INFINITY, -INFINITY, -INFINITY, 1);

    for (int i = 0; i < 8; ++i) {
        Vec4 p = m * corners[i];
        out.min.x = std::min(out.min.x, p.x);
        out.min.y = std::min(out.min.y, p.y);
        out.min.z = std::min(out.min.z, p.z);

        out.max.x = std::max(out.max.x, p.x);
        out.max.y = std::max(out.max.y, p.y);
        out.max.z = std::max(out.max.z, p.z);
    }

    return out;
}

Vec4 AABB::center() const {
    return (min + max) * 0.5f;
}

Vec4 AABB::extents() const {
    return (max - min) * 0.5f;
}
