#pragma once

#include "Vec4.h"
#include "Mtx4.h"
#include <cmath>
#include <algorithm>


#pragma once

#include "Vec4.h"
#include "Mtx4.h"
#include "Quaternion.h"
#include <cmath>
#include <algorithm>

/*
struct Camera {
public:
    Vec4 position;
    float nearZ;
    float farZ;
    float fovRad;

private:
    Vec4 target;   // stále držíme pro zpìtnou kompatibilitu
    Vec4 up;       // stále držíme pro zpìtnou kompatibilitu

    Quaternion orientation; // NOVÉ – nahrazuje yaw/pitch

    float yaw;     // zachováno kvùli API
    float pitch;   // zachováno kvùli API

public:

    Camera()
        : yaw(0.0f), pitch(0.0f),
        fovRad(0.0f), nearZ(0.0f), farZ(0.0f),
        orientation(Quaternion::Identity())
    {
    }

    Camera(const Vec4& pos, const Vec4& tar, const Vec4& u,
        float fov, float nZ, float fZ)
        : position(pos), target(tar), up(u),
        fovRad(fov), nearZ(nZ), farZ(fZ)
    {
        updateAnglesFromTarget();
        orientation = Quaternion::FromEuler(yaw, pitch, 0.0f);
    }

    // ---------------------------------------------------------
    // VIEW MATRIX
    // ---------------------------------------------------------
    Mtx4 calculateViewMatrix() const {
        return Mtx4::LookAt(position, target, up);
    }

    // ---------------------------------------------------------
    // ROTACE – API zachováno
    // ---------------------------------------------------------
    void rotate(float deltaYaw, float deltaPitch) {
        yaw += deltaYaw;
        pitch += deltaPitch;

        pitch = std::clamp(pitch, -1.55f, 1.55f);

        // pøepoèítáme quaternion
        //orientation = Quaternion::FromEuler(yaw, pitch, 0.0f);
        orientation =
            Quaternion::FromAxisAngle(Vec4(0, 1, 0, 0), yaw) *
            Quaternion::FromAxisAngle(Vec4(1, 0, 0, 0), pitch);


        updateTargetFromQuaternion();
    }

    // ---------------------------------------------------------
    // POHYB – API zachováno
    // direction je lokální
    // ---------------------------------------------------------
    void move(const Vec4& direction) {
        // transformace direction quaternionem
        Vec4 worldDir = orientation * direction;
        position = position + worldDir;

        updateTargetFromQuaternion();
    }

private:

    // ---------------------------------------------------------
    // Aktualizace target/up z quaternionu
    // ---------------------------------------------------------
    void updateTargetFromQuaternion() {
        Vec4 forward = orientation * Vec4(1, 0, 0, 0);
        forward.normalize();

        target = position + forward;

        Vec4 worldUp(0, 1, 0, 0);
        up = orientation * worldUp;
        up.normalize();
    }

    // ---------------------------------------------------------
    // Rekonstrukce yaw/pitch z targetu (kvùli API)
    // ---------------------------------------------------------
    void updateAnglesFromTarget() {
        Vec4 dir = (target - position).normalized();
        pitch = std::asin(dir.y);
        yaw = std::atan2(dir.z, dir.x);
    }

public:

    // ---------------------------------------------------------
    // DEMO kamera
    // ---------------------------------------------------------
    static Camera Demo(float fovDegrees = 45.0f) {
        Camera c;
        c.position = Vec4(5, 5, 5, 1);
        c.target = Vec4(0, 0, 0, 1);
        c.up = Vec4(0, 1, 0, 0);
        c.fovRad = fovDegrees * 3.14159265f / 180.0f;
        c.nearZ = 0.01f;
        c.farZ = 100.0f;

        c.updateAnglesFromTarget();
        //c.orientation = Quaternion::FromEuler(c.yaw, c.pitch, 0.0f);

        c.orientation = Quaternion::FromAxisAngle(Vec4(0, 1, 0, 0), c.yaw) * Quaternion::FromAxisAngle(Vec4(1, 0, 0, 0), c.pitch);


        return c;
    }

    // ---------------------------------------------------------
    // LERP – API zachováno
    // ---------------------------------------------------------
    Camera& operator=(const Camera& other) = default;

    friend Camera Lerp(const Camera& a, const Camera& b, float t) {
        Camera r;
        r.position = a.position + (b.position - a.position) * t;

        r.yaw = a.yaw + (b.yaw - a.yaw) * t;
        r.pitch = a.pitch + (b.pitch - a.pitch) * t;

        r.fovRad = a.fovRad + (b.fovRad - a.fovRad) * t;
        r.nearZ = a.nearZ + (b.nearZ - a.nearZ) * t;
        r.farZ = a.farZ + (b.farZ - a.farZ) * t;

        //r.orientation = Quaternion::FromEuler(r.yaw, r.pitch, 0.0f);
        r.orientation =
            Quaternion::FromAxisAngle(Vec4(0, 1, 0, 0), r.yaw) *
            Quaternion::FromAxisAngle(Vec4(1, 0, 0, 0), r.pitch);

        r.updateTargetFromQuaternion();
        return r;
    }

    // ---------------------------------------------------------
    // SLERP – API zachováno
    // ---------------------------------------------------------
    friend Camera Slerp(const Camera& a, const Camera& b, float t) {
        Camera r;

        r.position = a.position + (b.position - a.position) * t;

        // SLERP quaternionu
        r.orientation = Quaternion::Slerp(a.orientation, b.orientation, t);

        // Rekonstrukce yaw/pitch z quaternionu
        Vec4 forward = r.orientation * Vec4(1, 0, 0, 0);
        forward.normalize();

        r.pitch = std::asin(forward.y);
        r.yaw = std::atan2(forward.z, forward.x);

        r.fovRad = a.fovRad + (b.fovRad - a.fovRad) * t;
        r.nearZ = a.nearZ + (b.nearZ - a.nearZ) * t;
        r.farZ = a.farZ + (b.farZ - a.farZ) * t;

        r.updateTargetFromQuaternion();
        return r;
    }

};


/*/
struct Camera {
    // --- Data ---

public:
    Vec4 position;
    float nearZ;
    float farZ;
    float fovRad;
private:
    Vec4 target; // Vypoèítané API zpìtnì kompatibilní
    Vec4 up;     // Vypoèítané API zpìtnì kompatibilní

    
    

    // Vnitøní stav pro snadné ovládání
    float yaw;   // Otáèení vlevo/vpravo (radiány)
    float pitch; // Otáèení nahoru/dolù (radiány)
public:

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
//*/