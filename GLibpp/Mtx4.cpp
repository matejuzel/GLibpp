
#include "Mtx4.h"


    // identity
    Mtx4::Mtx4() {
        for (int i = 0; i < 16; i++)
            data[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }

    // from array
    Mtx4::Mtx4(const float* src) {
        for (int i = 0; i < 16; i++)
            data[i] = src[i];
    }

    // from 16 floats
    Mtx4::Mtx4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33
    ) {
        data[0] = m00; data[1] = m01; data[2] = m02; data[3] = m03;
        data[4] = m10; data[5] = m11; data[6] = m12; data[7] = m13;
        data[8] = m20; data[9] = m21; data[10] = m22; data[11] = m23;
        data[12] = m30; data[13] = m31; data[14] = m32; data[15] = m33;
    }

    float& Mtx4::at(int r, int c) { return data[r * 4 + c]; }
    float  Mtx4::at(int r, int c) const { return data[r * 4 + c]; }


    static Mtx4 multiply(const Mtx4& a, const Mtx4& b) {
        Mtx4 r;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                r.at(row, col) =
                    a.at(row, 0) * b.at(0, col) +
                    a.at(row, 1) * b.at(1, col) +
                    a.at(row, 2) * b.at(2, col) +
                    a.at(row, 3) * b.at(3, col);
            }
        }
        return r;
    }

    Mtx4 Mtx4::operator*(const Mtx4& m) const {
        Mtx4 r;

        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                r.at(row, col) =
                    at(row, 0) * m.at(0, col) +
                    at(row, 1) * m.at(1, col) +
                    at(row, 2) * m.at(2, col) +
                    at(row, 3) * m.at(3, col);
            }
        }

        return r;
    }

    Vec4 Mtx4::operator*(const Vec4& v) const {
        return Vec4(
            at(0, 0) * v.x + at(0, 1) * v.y + at(0, 2) * v.z + at(0, 3) * v.w,
            at(1, 0) * v.x + at(1, 1) * v.y + at(1, 2) * v.z + at(1, 3) * v.w,
            at(2, 0) * v.x + at(2, 1) * v.y + at(2, 2) * v.z + at(2, 3) * v.w,
            at(3, 0) * v.x + at(3, 1) * v.y + at(3, 2) * v.z + at(3, 3) * v.w
        );

    }

    Mtx4 Mtx4::operator+(const Mtx4& m) const {
        Mtx4 r;
        for (int i = 0; i < 16; i++) r.data[i] = data[i] + m.data[i];
        return r;
    }

    Mtx4 Mtx4::operator-(const Mtx4& m) const {
        Mtx4 r;
        for (int i = 0; i < 16; i++) r.data[i] = data[i] - m.data[i];
        return r;
    }

    Mtx4& Mtx4::operator*=(const Mtx4& m)
    {
        Mtx4 r;

        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                r.at(row, col) =
                    at(row, 0) * m.at(0, col) +
                    at(row, 1) * m.at(1, col) +
                    at(row, 2) * m.at(2, col) +
                    at(row, 3) * m.at(3, col);
            }
        }

        *this = r;
        return *this;
    }

    Mtx4& Mtx4::operator+=(const Mtx4& m) {
        for (int i = 0; i < 16; i++) data[i] += m.data[i];
        return *this;
    }

    Mtx4& Mtx4::operator-=(const Mtx4& m) {
        for (int i = 0; i < 16; i++) data[i] -= m.data[i];
        return *this;
    }

    Mtx4& Mtx4::transpose() {
        for (int r = 0; r < 4; r++) {
            for (int c = r + 1; c < 4; c++) {
                std::swap(at(r, c), at(c, r));
            }
        }
        return *this;
    }

    Mtx4& Mtx4::inverse()
    {
        Mtx4 inv;
        const float* m = data;

        inv.data[0] =
            m[5] * m[10] * m[15] -
            m[5] * m[11] * m[14] -
            m[9] * m[6] * m[15] +
            m[9] * m[7] * m[14] +
            m[13] * m[6] * m[11] -
            m[13] * m[7] * m[10];

        inv.data[4] =
            -m[4] * m[10] * m[15] +
            m[4] * m[11] * m[14] +
            m[8] * m[6] * m[15] -
            m[8] * m[7] * m[14] -
            m[12] * m[6] * m[11] +
            m[12] * m[7] * m[10];

        inv.data[8] =
            m[4] * m[9] * m[15] -
            m[4] * m[11] * m[13] -
            m[8] * m[5] * m[15] +
            m[8] * m[7] * m[13] +
            m[12] * m[5] * m[11] -
            m[12] * m[7] * m[9];

        inv.data[12] =
            -m[4] * m[9] * m[14] +
            m[4] * m[10] * m[13] +
            m[8] * m[5] * m[14] -
            m[8] * m[6] * m[13] -
            m[12] * m[5] * m[10] +
            m[12] * m[6] * m[9];

        inv.data[1] =
            -m[1] * m[10] * m[15] +
            m[1] * m[11] * m[14] +
            m[9] * m[2] * m[15] -
            m[9] * m[3] * m[14] -
            m[13] * m[2] * m[11] +
            m[13] * m[3] * m[10];

        inv.data[5] =
            m[0] * m[10] * m[15] -
            m[0] * m[11] * m[14] -
            m[8] * m[2] * m[15] +
            m[8] * m[3] * m[14] +
            m[12] * m[2] * m[11] -
            m[12] * m[3] * m[10];

        inv.data[9] =
            -m[0] * m[9] * m[15] +
            m[0] * m[11] * m[13] +
            m[8] * m[1] * m[15] -
            m[8] * m[3] * m[13] -
            m[12] * m[1] * m[11] +
            m[12] * m[3] * m[9];

        inv.data[13] =
            m[0] * m[9] * m[14] -
            m[0] * m[10] * m[13] -
            m[8] * m[1] * m[14] +
            m[8] * m[2] * m[13] +
            m[12] * m[1] * m[10] -
            m[12] * m[2] * m[9];

        inv.data[2] =
            m[1] * m[6] * m[15] -
            m[1] * m[7] * m[14] -
            m[5] * m[2] * m[15] +
            m[5] * m[3] * m[14] +
            m[13] * m[2] * m[7] -
            m[13] * m[3] * m[6];

        inv.data[6] =
            -m[0] * m[6] * m[15] +
            m[0] * m[7] * m[14] +
            m[4] * m[2] * m[15] -
            m[4] * m[3] * m[14] -
            m[12] * m[2] * m[7] +
            m[12] * m[3] * m[6];

        inv.data[10] =
            m[0] * m[5] * m[15] -
            m[0] * m[7] * m[13] -
            m[4] * m[1] * m[15] +
            m[4] * m[3] * m[13] +
            m[12] * m[1] * m[7] -
            m[12] * m[3] * m[5];

        inv.data[14] =
            -m[0] * m[5] * m[14] +
            m[0] * m[6] * m[13] +
            m[4] * m[1] * m[14] -
            m[4] * m[2] * m[13] -
            m[12] * m[1] * m[6] +
            m[12] * m[2] * m[5];

        inv.data[3] =
            -m[1] * m[6] * m[11] +
            m[1] * m[7] * m[10] +
            m[5] * m[2] * m[11] -
            m[5] * m[3] * m[10] -
            m[9] * m[2] * m[7] +
            m[9] * m[3] * m[6];

        inv.data[7] =
            m[0] * m[6] * m[11] -
            m[0] * m[7] * m[10] -
            m[4] * m[2] * m[11] +
            m[4] * m[3] * m[10] +
            m[8] * m[2] * m[7] -
            m[8] * m[3] * m[6];

        inv.data[11] =
            -m[0] * m[5] * m[11] +
            m[0] * m[7] * m[9] +
            m[4] * m[1] * m[11] -
            m[4] * m[3] * m[9] -
            m[8] * m[1] * m[7] +
            m[8] * m[3] * m[5];

        inv.data[15] =
            m[0] * m[5] * m[10] -
            m[0] * m[6] * m[9] -
            m[4] * m[1] * m[10] +
            m[4] * m[2] * m[9] +
            m[8] * m[1] * m[6] -
            m[8] * m[2] * m[5];

        float det =
            m[0] * inv.data[0] +
            m[1] * inv.data[4] +
            m[2] * inv.data[8] +
            m[3] * inv.data[12];

        if (det == 0.0f)
            (*this) = Mtx4(
                        0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f
                      ); // fallback: identity or zero matrix

        float invDet = 1.0f / det;
        for (int i = 0; i < 16; i++)
            inv.data[i] *= invDet;

        (*this) = inv;

        return *this;
    }

    Mtx4& Mtx4::inverseAffine()
    {
        Mtx4 r;

        // Invert 3x3 rotation/scale block (upper-left)
        float r00 = at(0, 0), r01 = at(0, 1), r02 = at(0, 2);
        float r10 = at(1, 0), r11 = at(1, 1), r12 = at(1, 2);
        float r20 = at(2, 0), r21 = at(2, 1), r22 = at(2, 2);

        // Compute determinant of 3x3
        float det =
            r00 * (r11 * r22 - r12 * r21) -
            r01 * (r10 * r22 - r12 * r20) +
            r02 * (r10 * r21 - r11 * r20);

        float invDet = 1.0f / det;

        // Inverse of 3x3 block
        r.at(0, 0) = (r11 * r22 - r12 * r21) * invDet;
        r.at(0, 1) = -(r01 * r22 - r02 * r21) * invDet;
        r.at(0, 2) = (r01 * r12 - r02 * r11) * invDet;

        r.at(1, 0) = -(r10 * r22 - r12 * r20) * invDet;
        r.at(1, 1) = (r00 * r22 - r02 * r20) * invDet;
        r.at(1, 2) = -(r00 * r12 - r02 * r10) * invDet;

        r.at(2, 0) = (r10 * r21 - r11 * r20) * invDet;
        r.at(2, 1) = -(r00 * r21 - r01 * r20) * invDet;
        r.at(2, 2) = (r00 * r11 - r01 * r10) * invDet;

        // Invert translation: -T * R^-1
        float tx = at(0, 3);
        float ty = at(1, 3);
        float tz = at(2, 3);

        r.at(0, 3) = -(tx * r.at(0, 0) + ty * r.at(0, 1) + tz * r.at(0, 2));
        r.at(1, 3) = -(tx * r.at(1, 0) + ty * r.at(1, 1) + tz * r.at(1, 2));
        r.at(2, 3) = -(tx * r.at(2, 0) + ty * r.at(2, 1) + tz * r.at(2, 2));

        // Last row stays the same
        r.at(3, 0) = 0;
        r.at(3, 1) = 0;
        r.at(3, 2) = 0;
        r.at(3, 3) = 1;

        *this = r;
        return *this;
    }


    Mtx4& Mtx4::translate(float x, float y, float z) {
        *this = *this * Mtx4::translation(x, y, z);
        return *this;
    }

    Mtx4& Mtx4::scale(float x, float y, float z) {
        *this = *this * Mtx4::scaling(x, y, z);
        return *this;
    }

    Mtx4& Mtx4::rotateX(float a) {
        *this = *this * Mtx4::rotationX(a);
        return *this;
    }

    Mtx4& Mtx4::rotateY(float a) {
        *this = *this * Mtx4::rotationY(a);
        return *this;
    }

    Mtx4& Mtx4::rotateZ(float a) {
        *this = *this * Mtx4::rotationZ(a);
        return *this;
    }

    Mtx4& Mtx4::multiply(const Mtx4& m)
    {
        (*this) *= m;
        return *this;
    }

    Mtx4& Mtx4::add(const Mtx4& m)
    {
        (*this) += m;
        return *this;
    }

    Mtx4& Mtx4::subtract(const Mtx4& m)
    {
        (*this) -= m;
        return *this;
    }

    Mtx4& Mtx4::multiply(float k)
    {
        for (int i = 0; i < 16; i++)
            data[i] *= k;

        return *this;
    }

    Mtx4 Mtx4::transposed() const
    {
        Mtx4 r = *this;
        r.transpose();
        return r;
    }

    Mtx4 Mtx4::inverted() const
    {
        Mtx4 r = *this;
        r.inverse();
        return r;
    }

    Mtx4 Mtx4::invertedAffine() const
    {
        Mtx4 r = *this;
        r.inverseAffine();
        return r;
    }


    Mtx4 Mtx4::translation(float x, float y, float z) {
        return Mtx4(
            1, 0, 0, x,
            0, 1, 0, y,
            0, 0, 1, z,
            0, 0, 0, 1
        );
    }


    Mtx4 Mtx4::scaling(float x, float y, float z) {
        return Mtx4(
            x, 0, 0, 0,
            0, y, 0, 0,
            0, 0, z, 0,
            0, 0, 0, 1
        );
    }

    Mtx4 Mtx4::rotationX(float a) {
        float c = cos(a), s = sin(a);
        return Mtx4(
            1, 0, 0, 0,
            0, c, -s, 0,
            0, s, c, 0,
            0, 0, 0, 1
        );
    }

    Mtx4 Mtx4::rotationY(float a) {
        float c = cos(a), s = sin(a);
        return Mtx4(
            c, 0, s, 0,
            0, 1, 0, 0,
            -s, 0, c, 0,
            0, 0, 0, 1
        );
    }

    Mtx4 Mtx4::rotationZ(float a) {
        float c = cos(a), s = sin(a);
        return Mtx4(
            c, -s, 0, 0,
            s, c, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        );
    }

    Mtx4 Mtx4::lookAt(const Vec4& eye, const Vec4& target, const Vec4& up) {
        Vec4 f = Vec4::normalize(target - eye);
        Vec4 s = Vec4::normalize(Vec4::cross(f, up));
        Vec4 u = Vec4::cross(s, f);

        return Mtx4(
            s.x, s.y, s.z, -Vec4::dot(s, eye),
            u.x, u.y, u.z, -Vec4::dot(u, eye),
            -f.x, -f.y, -f.z, Vec4::dot(f, eye),
            0, 0, 0, 1
        );
    }

    Mtx4 Mtx4::perspective(float fov, float aspect, float nearZ, float farZ) {
        float f = 1.0f / tan(fov * 0.5f);
        return Mtx4(
            f / aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, farZ / (farZ - nearZ), (-farZ * nearZ) / (farZ - nearZ),
            0, 0, 1, 0
        );
    }

    Mtx4 Mtx4::orthographic(float left, float right, float bottom, float top, float nearZ, float farZ) {
        return Mtx4(
            2 / (right - left), 0, 0, -(right + left) / (right - left),
            0, 2 / (top - bottom), 0, -(top + bottom) / (top - bottom),
            0, 0, 1 / (farZ - nearZ), -nearZ / (farZ - nearZ),
            0, 0, 0, 1
        );
    }

    std::string Mtx4::toString() const {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3);

        for (int r = 0; r < 4; r++) {
            ss << "[ ";
            for (int c = 0; c < 4; c++) {
                ss << at(r, c);
                if (c < 3) ss << ", ";
            }
            ss << " ]\n";
        }

        return ss.str();
    }
