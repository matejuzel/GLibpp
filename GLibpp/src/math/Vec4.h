#pragma once
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

class Vec4 {
public:
    float x, y, z, w;

    Vec4() : x(0), y(0), z(0), w(1) {}
    Vec4(float x, float y, float z = 0.0f, float w = 1.0f)
        : x(x), y(y), z(z), w(w) {
    }

    Vec4(int x, int y, int z = 0, int w = 1)
        : x(static_cast<float>(x)), y(static_cast<float>(y)), z(static_cast<float>(z)), w(static_cast<float>(w)) {}


    float length() const;
    Vec4 normalized() const;
    Vec4& normalize();
    static float dot(const Vec4& a, const Vec4& b);
    static Vec4 cross(const Vec4& a, const Vec4& b);
    static Vec4 normalize(const Vec4& v);

    float dot(const Vec4& v) const;
    Vec4& cross(const Vec4& v);

    Vec4 operator+(const Vec4& o) const;
    Vec4 operator-(const Vec4& o) const;
    Vec4 operator*(float s) const;
    std::string toString() const;


    Vec4& divideW() {
        x /= w;
        y /= w;
        z /= w;
        w = 1.0f;
        return *this;
    }


    friend inline Vec4 cross(const Vec4& a, const Vec4& b) 
    {
        return Vec4::cross(a, b);
    }

    static Vec4 Slerp(const Vec4& a, const Vec4& b, float t)
    {
        // 1) Normalizace vstupù (pro jistotu)
        Vec4 v0 = a.normalized();
        Vec4 v1 = b.normalized();

        // 2) Dot produkt
        float dot = Vec4::dot(v0, v1);

        // 3) Pokud jsou skoro rovnobìžné -> použij LERP
        const float EPS = 1e-6f;
        if (std::fabs(dot) > 1.0f - EPS)
        {
            // LERP + normalizace
            return (v0 * (1.0f - t) + v1 * t).normalized();
        }

        // 4) Úhel mezi vektory
        float theta = std::acos(dot);

        // 5) Výpoèet vah
        float sinTheta = std::sin(theta);
        float w0 = std::sin((1.0f - t) * theta) / sinTheta;
        float w1 = std::sin(t * theta) / sinTheta;

        // 6) Výsledek
        return v0 * w0 + v1 * w1;
    }

    static Vec4 Lerp(const Vec4& a, const Vec4& b, float t)
    {
        return a * (1.0f - t) + b * t;
    }


};


