#pragma once
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

class Vec4 {
public:
    float x, y, z, w;

    Vec4() : x(0), y(0), z(0), w(1) {}
    Vec4(float x, float y, float z, float w = 1.0f)
        : x(x), y(y), z(z), w(w) {
    }

    float length() const;
    Vec4 normalized() const;
    Vec4& normalize();
    static float dot(const Vec4& a, const Vec4& b);
    static Vec4 cross(const Vec4& a, const Vec4& b);
    static Vec4 normalize(const Vec4& v);

    float dot(const Vec4& v);
    Vec4& cross(const Vec4& v);

    Vec4 operator+(const Vec4& o) const;
    Vec4 operator-(const Vec4& o) const;
    Vec4 operator*(float s) const;
    std::string toString() const;

};


