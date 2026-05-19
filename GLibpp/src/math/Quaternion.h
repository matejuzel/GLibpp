#pragma once

#include "Vec4.h"
#include "Mtx4.h"

struct Quaternion
{
    float x, y, z, w;

    // identity
    Quaternion() : x(0), y(0), z(0), w(1) {}

    Quaternion(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {
    }

    static Quaternion Identity()
    {
        return Quaternion(0, 0, 0, 1);
    }

    static Quaternion FromAxisAngle(const Vec4& axis, float angle)
    {
        float half = angle * 0.5f;
        float s = sin(half);
        return Quaternion(axis.x * s, axis.y * s, axis.z * s, cos(half));
    }

    static Quaternion FromEuler(float yaw, float pitch, float roll) noexcept
    {
        float cy = cosf(yaw * 0.5f);
        float sy = sinf(yaw * 0.5f);
        float cp = cosf(pitch * 0.5f);
        float sp = sinf(pitch * 0.5f);
        float cr = cosf(roll * 0.5f);
        float sr = sinf(roll * 0.5f);

        Quaternion q;
        q.w = cr * cp * cy + sr * sp * sy;
        q.x = sr * cp * cy - cr * sp * sy;
        q.y = cr * sp * cy + sr * cp * sy;
        q.z = cr * cp * sy - sr * sp * cy;
        return q;
    }

    void normalize()
    {
        float len = length();
        if (len == 0.0f) {
            x = 0;
            y = 0;
            z = 0;
            w = 1;
            return;
        }
        float inv = 1.0f / len;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
    }

    Quaternion normalized() const
    {
        float len = length();
        if (len == 0.0f) return Quaternion(0, 0, 0, 1);
        float inv = 1.0f / len;
        return Quaternion(x * inv, y * inv, z * inv, w * inv);
    }

    bool isNormalized(float eps = 1e-3f) const noexcept
    {
        return fabsf(lengthSquared() - 1.0f) < eps;
    }

    Quaternion operator*(const Quaternion& q) const
    {
        return Quaternion(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        );
    }

    Vec4 operator*(const Vec4& v) const noexcept
    {
        // q = (x, y, z, w)
        Vec4 qv(x, y, z, 0.0f);
        Vec4 p(v.x, v.y, v.z, 0.0f);

        // t = 2 * cross(q.xyz, p)
        Vec4 t = cross(qv, p) * 2.0f;

        // rotated = p + w * t + cross(q.xyz, t)
        Vec4 r = p + t * w + cross(qv, t);

        return Vec4(r.x, r.y, r.z, v.w); // w se nemìní
    }


    /* // zatim nemame Vec3
    Vec3 operator*(const Vec3& v) const noexcept
    {
        // q = (x, y, z, w)
        Vec3 qv(x, y, z);

        // t = 2 * cross(q.xyz, v)
        Vec3 t = 2.0f * cross(qv, v);

        // v' = v + w * t + cross(q.xyz, t)
        return v + w * t + cross(qv, t);
    }
    */


    float length() const noexcept
    {
        return sqrtf(x * x + y * y + z * z + w * w);
    }

    float lengthSquared() const noexcept
    {
        return x * x + y * y + z * z + w * w;
    }

    Quaternion invert() const
    {
        // spoèítáme |q|^2
        float len2 = lengthSquared();

        // pokud je quaternion témìø normalizovaný
        if (fabs(len2 - 1.0f) < 1e-3f)
        {
            // rychlá inverze = konjugace
            return Quaternion(-x, -y, -z, w);
        }

        // jinak bezpeèná inverze
        if (len2 == 0.0f)
            return Quaternion(0, 0, 0, 1); // fallback

        float inv = 1.0f / len2;
        return Quaternion(-x * inv, -y * inv, -z * inv, w * inv);
    }

    static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) noexcept
    {
        // dot product
        float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

        // pokud je dot < 0 -> invertujeme b, aby interpolace šla nejkratší cestou
        Quaternion bb = b;
        if (dot < 0.0f) {
            dot = -dot;
            bb.x = -bb.x;
            bb.y = -bb.y;
            bb.z = -bb.z;
            bb.w = -bb.w;
        }

        // pokud jsou quaterniony velmi blízko -> použijeme LERP
        if (dot > 0.9995f) {
            // lineární interpolace + normalizace
            Quaternion r(
                a.x + t * (bb.x - a.x),
                a.y + t * (bb.y - a.y),
                a.z + t * (bb.z - a.z),
                a.w + t * (bb.w - a.w)
            );
            r.normalize();
            return r;
        }

        // SLERP
        float theta = acosf(dot);
        float sinTheta = sinf(theta);

        float w1 = sinf((1.0f - t) * theta) / sinTheta;
        float w2 = sinf(t * theta) / sinTheta;

        return Quaternion(
            a.x * w1 + bb.x * w2,
            a.y * w1 + bb.y * w2,
            a.z * w1 + bb.z * w2,
            a.w * w1 + bb.w * w2
        );
    }


    // pøevod na 4×4 matici
    Mtx4 toMatrix() const
    {
        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        return Mtx4(
            1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy), 0,
            2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx), 0,
            2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy), 0,
            0, 0, 0, 1
        );
    }
};
