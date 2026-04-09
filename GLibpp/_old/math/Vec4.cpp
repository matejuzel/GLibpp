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

float Vec4::dot(const Vec4& v)
{   
    return Vec4::dot(*this, v);
}

Vec4& Vec4::cross(const Vec4& v)
{
    *this = Vec4::cross(*this, v);
    return *this;
}

Vec4 Vec4::operator+(const Vec4& o) const {
    return Vec4(x + o.x, y + o.y, z + o.z, w + o.w);
}

Vec4 Vec4::operator-(const Vec4& o) const {
    return Vec4(x - o.x, y - o.y, z - o.z, w - o.w);
}

// Násobení skalárem (v * s)
Vec4 Vec4::operator*(float s) const {
    return Vec4(x * s, y * s, z * s, w * s);
}


std::string Vec4::toString() const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);

    float values[4] = { x, y, z, w };

    // 1) Najdi nejdelší øetìzec (vèetnì znaménka)
    int maxWidth = 0;
    for (int i = 0; i < 4; i++) {
        std::ostringstream tmp;
        tmp << std::fixed << std::setprecision(3)
            << (values[i] >= 0 ? "+" : "")  // pøidej + pro kladná èísla
            << values[i];
        int len = (int)tmp.str().length();
        if (len > maxWidth)
            maxWidth = len;
    }

    // 2) Vypiš zarovnanì
    std::ostringstream out;
    out << std::fixed << std::setprecision(3);

    out << "[ ";
    for (int i = 0; i < 4; i++) {
        out << std::setw(maxWidth)
            << (values[i] >= 0 ? "+" : "")  // stejné pravidlo pøi výpisu
            << values[i];
        if (i < 3) out << ", ";
    }
    out << " ]";

    return out.str();
}

