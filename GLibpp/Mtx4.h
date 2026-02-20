#pragma once
#include <string>
#include <cmath>
#include "Vec4.h"


#pragma once
#include <string>
#include <cmath>
#include "Vec4.h"

class Mtx4 {
public:
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

    float& at(int r, int c);
    float  at(int r, int c) const;

    Mtx4 operator*(const Mtx4& m) const;
    Vec4 operator*(const Vec4& v) const;
    Mtx4 operator+(const Mtx4& m) const;
    Mtx4 operator-(const Mtx4& m) const;

    Mtx4& operator*=(const Mtx4& m);
    Mtx4& operator+=(const Mtx4& m);
    Mtx4& operator-=(const Mtx4& m);

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
    
    static Mtx4 translation(float x, float y, float z);
    static Mtx4 scaling(float x, float y, float z);
    static Mtx4 rotationX(float a);
    static Mtx4 rotationY(float a);
    static Mtx4 rotationZ(float a);
    static Mtx4 lookAt(const Vec4& eye, const Vec4& target, const Vec4& up);
    static Mtx4 perspective(float fov, float aspect, float nearZ, float farZ);
    static Mtx4 orthographic(float left, float right, float bottom, float top, float nearZ, float farZ);

    float determinant() const;

    std::string toString() const;
    std::string toStringDetail() const;
};

