#pragma once

#include "Vec4.h"
#include "Mtx4.h"

#include <cmath>
#include <algorithm>

struct Camera {
    Vec4 position;
    Vec4 target;
    Vec4 up;

    float fovRad;
    float nearZ;
    float farZ;

    Camera(
        const Vec4& position, 
        const Vec4& target, 
        const Vec4& up, 
        float fovRad, 
        float nearZ, 
        float farZ
    )
        : position(position)
        , target(target)
        , up(up)
        , fovRad(fovRad)
        , nearZ(nearZ)
        , farZ(farZ) 
    {}

    Mtx4 calculateView() const
    {
        return Mtx4::LookAt(position, target, up);
	}

    static Camera Demo(float fovDegrees = 45.0f)
    {
        return Camera(
            Vec4(5.0f, 5.0f, 5.0f, 1.0f),
            Vec4(0.0f, 0.0f, 0.0f, 1.0f),
            Vec4(0.0f, 1.0f, 0.0f, 0.0f),
            fovDegrees * 3.14159265f / 180.0f,
            0.01f,
            100.0f
        );
    }
    static Camera Demo(int fovDegrees = 45) {
		return Demo(static_cast<float>(fovDegrees));
    }

    friend Camera Lerp(const Camera& a, const Camera& b, float t)
    {
        // Klasický Lerp: start + t * (end - start)
        // Pøedpokládám, že tvùj Vec4 má definované operátory + a *
        return Camera(
            a.position + (b.position - a.position) * t,
            a.target + (b.target - a.target) * t,
            a.up + (b.up - a.up) * t,
            a.fovRad + (b.fovRad - a.fovRad) * t,
            a.nearZ + (b.nearZ - a.nearZ) * t,
            a.farZ + (b.farZ - a.farZ) * t
        );
    }

    friend Camera Slerp(const Camera& a, const Camera& b, float t)
    {
        // 1. Lineární interpolace pro pozice a parametry èoèky
        Vec4 pos = a.position + (b.position - a.position) * t;
        Vec4 target = a.target + (b.target - a.target) * t;
        float fov = a.fovRad + (b.fovRad - a.fovRad) * t;
        float nZ = a.nearZ + (b.nearZ - a.nearZ) * t;
        float fZ = a.farZ + (b.farZ - a.farZ) * t;

        // 2. Sférická interpolace (Slerp) pro vektor UP
        // Pøedpokládáme, že a.up a b.up jsou normalizované.
        float dot = a.up.dot(b.up);

        // Ošetøení numerické pøesnosti pro acos
        dot = std::clamp(dot, -1.0f, 1.0f);

        float theta = std::acos(dot) * t;
        Vec4 relativeVec = (b.up - a.up * dot);
        relativeVec.normalize();

        // Výsledný rotovaný vektor UP
        Vec4 up = (a.up * std::cos(theta)) + (relativeVec * std::sin(theta));

        return Camera(pos, target, up, fov, nZ, fZ);
    }
};