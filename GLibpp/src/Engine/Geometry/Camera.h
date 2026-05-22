#pragma once

#include "Vec4.h"
#include "Mtx4.h"
#include <cmath>
#include <algorithm>

struct Camera {
    // --- Data ---
    Vec4 position;
    Vec4 target; // Vypoèítané API zpìtnì kompatibilní
    Vec4 up;     // Vypoèítané API zpìtnì kompatibilní

    float fovRad;
    float nearZ;
    float farZ;

    // Vnitøní stav pro snadné ovládání
    float yaw;   // Otáèení vlevo/vpravo (radiány)
    float pitch; // Otáèení nahoru/dolù (radiány)

    // --- Konstruktory ---
    Camera() : yaw(0.0f), pitch(0.0f), fovRad(0.0f), nearZ(0.0f), farZ(0.0f) {}

    Camera(const Vec4& pos, const Vec4& tar, const Vec4& u, float fov, float nZ, float fZ)
        : position(pos), target(tar), up(u), fovRad(fov), nearZ(nZ), farZ(fZ)
    {
        // Pøi inicializaci pøes LookAt parametry musíme dopoèítat úhly yaw/pitch
        updateAnglesFromTarget();
    }

    // --- Klíèové API pro výpoèet View Matice ---
    Mtx4 calculateViewMatrix() const {
        return Mtx4::LookAt(position, target, up);
    }

    // --- Ovládání (pro Input systém) ---

    // Otáèení (deltaX/deltaY z myši)
    void rotate(float deltaYaw, float deltaPitch) {
        yaw += deltaYaw;
        pitch += deltaPitch;

        // Omezení pohledu nahoru/dolù (ochrana pøed gimbal lockem v LookAt)
        pitch = std::clamp(pitch, -1.55f, 1.55f);

        updateTargetFromAngles();
    }

    // Pohyb (W, A, S, D z klávesnice)
    void move(const Vec4& direction) {
        // direction je lokální (napø. 1,0,0 pro dopøedu), musíme ho transformovat
        // nebo jednodušeji:
        position = position + direction;
        updateTargetFromAngles(); // Target se musí posunout s námi
    }

    // --- Interní logika ---

    void updateTargetFromAngles() {
        // Sférické souøadnice -> Kartézské souøadnice (Forward vektor)
        Vec4 forward;
        forward.x = std::cos(yaw) * std::cos(pitch);
        forward.y = std::sin(pitch);
        forward.z = std::sin(yaw) * std::cos(pitch);
        forward.w = 0.0f;
        forward.normalize();

        // Target je prostì bod kousek pøed kamerou
        target = position + forward;

        // Up vektor (zde fixní world up, pro stabilní FPS kameru)
        // Pro náklony (Roll) by se muselo slerpovat i Up
        Vec4 worldUp(0.0f, 1.0f, 0.0f, 0.0f);
        Vec4 right = worldUp.cross(forward).normalized();
        up = forward.cross(right).normalized();
    }

    void updateAnglesFromTarget() {
        Vec4 dir = (target - position).normalized();
        pitch = std::asin(dir.y);
        yaw = std::atan2(dir.z, dir.x);
    }

    // --- Statické továrny ---
    static Camera Demo(float fovDegrees = 45.0f) {
        Camera c;
        c.position = Vec4(5.0f, 5.0f, 5.0f, 1.0f);
        c.target = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        c.up = Vec4(0.0f, 1.0f, 0.0f, 0.0f);
        c.fovRad = fovDegrees * 3.14159265f / 180.0f;
        c.nearZ = 0.01f;
        c.farZ = 100.0f;
        c.updateAnglesFromTarget();
        return c;
    }

    // --- Operátory a Interpolace (zùstávají stejné) ---
    Camera& operator=(const Camera& other) = default;

    friend Camera Lerp(const Camera& a, const Camera& b, float t) {
        Camera res;
        res.position = a.position + (b.position - a.position) * t;
        res.yaw = a.yaw + (b.yaw - a.yaw) * t;
        res.pitch = a.pitch + (b.pitch - a.pitch) * t;
        res.fovRad = a.fovRad + (b.fovRad - a.fovRad) * t;
        res.nearZ = a.nearZ + (b.nearZ - a.nearZ) * t;
        res.farZ = a.farZ + (b.farZ - a.farZ) * t;
        res.updateTargetFromAngles();
        return res;
    }

    friend Camera Slerp(const Camera& a, const Camera& b, float t) {
        // U moderní kamery je lepší interpolovat Yaw/Pitch než Target!
        Camera res = Lerp(a, b, t);
        // Pokud chceš slerpovat orientaci báze (Up), použij tu pùvodní Slerp logiku,
        // ale pro FPS kameru je Lerp úhlù Yaw/Pitch vizuálnì plynulejší.
        return res;
    }
};