#pragma once

#include "math/Vec4.h"

class Camera {
private:
    float fov;
    float aspect;
    float nearPlane;
    float farPlane;

    Vec4 position;
    Vec4 target;
    Vec4 up;

public:
    Camera(float fov, float aspect, float nearPlane, float farPlane, const Vec4& eye, const Vec4& center, const Vec4& up)
        : fov(fov), aspect(aspect), nearPlane(nearPlane), farPlane(farPlane),
        position(eye), target(center), up(up)
    {
    }

    Camera()
        : fov(45.0f), aspect(16.0f / 9.0f), nearPlane(0.01f), farPlane(50.0f),
        position(1.0f, 1.0f, 5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f)
    {
    }

    Mtx4 getViewMatrix() const {
        return Mtx4::lookAt(
            Vec4(position),
            Vec4(target),
            Vec4(up)
        );
    }


    Mtx4 getProjectionMatrix() const {
        return Mtx4::perspective(fov, aspect, nearPlane, farPlane);
    }
};
