#include "Vec4.h"

float Vec4::length() const {
    return std::sqrt(x * x + y * y + z * z + w * w);
}

Vec4 Vec4::normalized() const {
    float len = length();
    if (len == 0.0f) return Vec4(0, 0, 0, 0);
    return Vec4(x / len, y / len, z / len, w / len);
}

Vec4& Vec4::normalize() {
    float len = length();
    if (len == 0.0f) return *this;
    x /= len;
    y /= len;
    z /= len;
    w /= len;
    return *this;
}

float Vec4::dot(const Vec4& a, const Vec4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vec4 Vec4::cross(const Vec4& a, const Vec4& b) {
    // Cross product ignores w (treats vectors as 3D)
    return Vec4(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
        0.0f
    );
}

Vec4 Vec4::normalize(const Vec4& v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    if (len == 0.0f) return Vec4(0, 0, 0, 0);
    return Vec4(v.x / len, v.y / len, v.z / len, v.w / len);
}

Vec4 Vec4::operator-(const Vec4& o) const {
    return Vec4(x - o.x, y - o.y, z - o.z, w - o.w);
}

std::string Vec4::toString() const {
    std::ostringstream ss;
    ss << "[" << x << ", " << y << ", " << z << ", " << w << "]";
    return ss.str();
}