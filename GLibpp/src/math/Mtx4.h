#pragma once
#include <string>
#include <cstring>
#include <cmath>
#include "Vec4.h"

#include <algorithm>

struct Mtx4 {
    float data[16];

    // identity
    Mtx4();

    // from array
    Mtx4(const float* src);

    // from 16 floats
    Mtx4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33
    );

    // special member functions — all defaulted
    Mtx4(const Mtx4&) = default;
    Mtx4(Mtx4&&) = default;
    Mtx4& operator=(const Mtx4&) = default;
    Mtx4& operator=(Mtx4&&) = default;
    ~Mtx4() = default;

	const float* getRawData() const 
    { 
        return data;
    }

    float& at(int r, int c);
    float  at(int r, int c) const;

    Mtx4 operator*(const Mtx4& m) const;
    Vec4 operator*(const Vec4& v) const;
    Mtx4 operator+(const Mtx4& m) const;
    Mtx4 operator-(const Mtx4& m) const;

    Mtx4& operator*=(const Mtx4& m);
    Mtx4& operator+=(const Mtx4& m);
    Mtx4& operator-=(const Mtx4& m);

    Mtx4& setIdentity();
    Mtx4& inverse();
    Mtx4& inverseAffine();
    Mtx4& transpose();
    Mtx4& translate(float x, float y, float z);
    Mtx4& scale(float x, float y, float z);
    Mtx4& rotateX(float a);
    Mtx4& rotateY(float a);
    Mtx4& rotateZ(float a);
    Mtx4& multiply(const Mtx4& m);
    Mtx4& add(const Mtx4& m);
    Mtx4& subtract(const Mtx4& m);
    Mtx4& multiply(float k);

    Mtx4 transposed() const;
    Mtx4 inverted() const;
    Mtx4 invertedAffine() const;
    
    static Mtx4 Identity();
    static Mtx4 Translation(float x, float y, float z);
    static Mtx4 Scaling(float x, float y, float z);
    static Mtx4 RotationX(float a);
    static Mtx4 RotationY(float a);
    static Mtx4 RotationZ(float a);
    static Mtx4 LookAt(const Vec4& eye, const Vec4& target, const Vec4& up);
    static Mtx4 Perspective(float fov, float aspect, float nearZ, float farZ);
    static Mtx4 Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ);

    float determinant() const;

    std::string toString() const;
    std::string toStringDetail() const;


    static Mtx4 Slerp(const Mtx4& a, const Mtx4& b, float t) {
        auto slerpVec = [](const Vec4& v1, const Vec4& v2, float t) -> Vec4 {
            float dot = std::clamp(v1.dot(v2), -1.0f, 1.0f);
            if (dot > 0.9999f) return v1;
            if (dot < -0.9999f) return (v1 * (1.0f - t) + v2 * t).normalized();

            float theta_0 = std::acos(dot);
            float s1 = std::sin((1.0f - t) * theta_0) / std::sin(theta_0);
            float s2 = std::sin(t * theta_0) / std::sin(theta_0);
            return (v1 * s1) + (v2 * s2);
            };

        // 1. EXTRAKCE (Row-Major indexy: X=0,4,8; Y=1,5,9; Z=2,6,10)
        Vec4 sideA(a.data[0], a.data[4], a.data[8], 0.0f);
        Vec4 upA(a.data[1], a.data[5], a.data[9], 0.0f);
        Vec4 fwdA(a.data[2], a.data[6], a.data[10], 0.0f);

        Vec4 sideB(b.data[0], b.data[4], b.data[8], 0.0f);
        Vec4 upB(b.data[1], b.data[5], b.data[9], 0.0f);
        Vec4 fwdB(b.data[2], b.data[6], b.data[10], 0.0f);

        // 2. INTERPOLACE SMÌRÙ
        Vec4 fwd = slerpVec(fwdA.normalized(), fwdB.normalized(), t);
        Vec4 up = slerpVec(upA.normalized(), upB.normalized(), t);

        // 3. ORTONORMALIZACE (Køížový souèin pro pravotoèivou soustavu)
        Vec4 right = up.cross(fwd).normalized();
        up = fwd.cross(right).normalized();

        // 4. INTERPOLACE POZICE (Row-Major: translace je obvykle v posledním sloupci 3,7,11)
        float om = 1.0f - t;
        Vec4 pos(
            a.data[3] * om + b.data[3] * t,
            a.data[7] * om + b.data[7] * t,
            a.data[11] * om + b.data[11] * t,
            1.0f
        );

        // 5. SESTAVENÍ (Zpìt do Row-Major layoutu)
        Mtx4 res;
        // Øádek 0
        res.data[0] = right.x; res.data[1] = up.x;   res.data[2] = fwd.x;  res.data[3] = pos.x;
        // Øádek 1
        res.data[4] = right.y; res.data[5] = up.y;   res.data[6] = fwd.y;  res.data[7] = pos.y;
        // Øádek 2
        res.data[8] = right.z; res.data[9] = up.z;   res.data[10] = fwd.z; res.data[11] = pos.z;
        // Øádek 3
        res.data[12] = 0.0f;    res.data[13] = 0.0f;  res.data[14] = 0.0f;  res.data[15] = 1.0f;

        return res;
    }

};

