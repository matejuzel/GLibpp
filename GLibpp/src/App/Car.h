#pragma once

#include <cmath>

#include "Vec4.h"
#include "Mtx4.h"
#include "Quaternion.h"
#include "Mathematics.h"
#include "BicycleModel.h"

// auto a jeho kola - herni stav a chovani nad fyzikalnim BicycleModelem

struct CarWheel {

    CarWheel(float zPos, float xPos)
        : position(Vec4(xPos, 0.0f, zPos, 1.0f))
    {
    }

    void doRoll(float dAngle) {
        // uhel drzime wrapnuty v [0, 2*pi) - neomezene rostouci rollAngle by po delsi
        // jizde ztratil float presnost a kola by viditelne jitterovala
        rollAngle = std::fmod(rollAngle + dAngle, kTwoPi);
        if (rollAngle < 0.0f) rollAngle += kTwoPi;
    }

    Mtx4 get() const {

        auto m_pos = Mtx4::Identity().translate(position.x, position.y, position.z);
        auto m_steer = Mtx4::Identity().rotateY(steerAngle);
        auto m_roll = Mtx4::Identity().rotateX(rollAngle);

        return m_pos * m_steer * m_roll;
    }

    static CarWheel Lerp(const CarWheel& a, const CarWheel& b, float t) {

        auto interpolated = b;

        interpolated.steerAngle = a.steerAngle + (b.steerAngle - a.steerAngle) * t;

        // rollAngle je wrapnuty -> interpolujeme po nejkratsi ceste pres hranici 0/2*pi
        float dRoll = std::remainder(b.rollAngle - a.rollAngle, kTwoPi);
        interpolated.rollAngle = a.rollAngle + dRoll * t;

        return interpolated;
    }

    float steerAngle = 0.0f; // rad
    float rollAngle = 0.0f;  // rad, wrapnuty v [0, 2*pi)

private:

    static constexpr float kTwoPi = static_cast<float>(2.0 * GLibpp::Math::pi);

    Vec4 position;
};


struct Car
{
    // fyzikální model – poloha = střed zadní nápravy
    GLibpp::Physics::BicycleModel model;

    // wheel transformace
    CarWheel wheelFrontLeft;
    CarWheel wheelFrontRight;
    CarWheel wheelBackLeft;
    CarWheel wheelBackRight;

    // geometrie uz zde neni - meshe vlastni ResourceManager (App::setupDemoResources),
    // Scene nese jen handly (SceneRenderables) a transformace -> kopie Scene nealokuji

    Car()
        : model(GLibpp::Physics::BicycleModel::Basic())
        , wheelFrontLeft(model.params.wheelBase, -model.params.wheelTrack * 0.5f)
        , wheelFrontRight(model.params.wheelBase, model.params.wheelTrack * 0.5f)
        , wheelBackLeft(0, -model.params.wheelTrack * 0.5f)
        , wheelBackRight(0, model.params.wheelTrack * 0.5f)
    {}

    void speedUp(float dSpeed)
    {
        model.accelerate(dSpeed);
    }

    void speedDown(float faktor)
    {
        model.brake(faktor);
    }

    void steerFrontWheels(float dAngle)
    {
        model.steer(dAngle);
        wheelFrontLeft.steerAngle = model.getSteerAngleLeft();
        wheelFrontRight.steerAngle = model.getSteerAngleRight();
    }

    void steerFrontWheelsReset(float dt)
    {
        model.steerReset(dt);
        wheelFrontLeft.steerAngle = model.getSteerAngleLeft();
        wheelFrontRight.steerAngle = model.getSteerAngleRight();
    }

    Mtx4 getCarMatrix() const
    {
        return model.getTransformation();
    }

    Mtx4 getIcrTransformation() const {

        Mtx4 m = Mtx4::Identity();
        m.translate(model.getIcr(), 0.0f, 0.0f);
        return model.getTransformation() * m;
    }

    void run(float dt)
    {
        // fyzika
        model.update(dt);

        // roll kol
        rollAllWheels(dt);

        // vizuální steer kol
        wheelFrontLeft.steerAngle = model.getSteerAngleLeft();
        wheelFrontRight.steerAngle = model.getSteerAngleRight();
    }

    void rollAllWheels(float dt)
    {
        float v = model.getSpeed();
        float icr = model.getIcr();
        float r = model.params.wheelRadius;

        // rovná jízda (icr = inf pri nulovem rejdu)
        if (!std::isfinite(icr)) {
            float w = v / r;
            wheelFrontLeft.doRoll(w * dt);
            wheelFrontRight.doRoll(w * dt);
            wheelBackLeft.doRoll(w * dt);
            wheelBackRight.doRoll(w * dt);
            return;
        }

        float turnSign = (icr > 0 ? +1.0f : -1.0f);
        float Omega = v / icr;

        // rychlosti kol
        float v_FL = turnSign * Omega * model.getIcrFL();
        float v_FR = turnSign * Omega * model.getIcrFR();
        float v_BL = turnSign * Omega * model.getIcrBL();
        float v_BR = turnSign * Omega * model.getIcrBR();

        // úhlové rychlosti kol
        wheelFrontLeft.doRoll((v_FL / r) * dt);
        wheelFrontRight.doRoll((v_FR / r) * dt);
        wheelBackLeft.doRoll((v_BL / r) * dt);
        wheelBackRight.doRoll((v_BR / r) * dt);
    }

    Mtx4 getFrontLeft()  const { return getCarMatrix() * wheelFrontLeft.get(); }
    Mtx4 getFrontRight() const { return getCarMatrix() * wheelFrontRight.get(); }
    Mtx4 getBackLeft()   const { return getCarMatrix() * wheelBackLeft.get(); }
    Mtx4 getBackRight()  const { return getCarMatrix() * wheelBackRight.get(); }


    friend Car Slerp(const Car& a, const Car& b, float t) {

        Car interpolated = a;

        interpolated.model.heading = Quaternion::Slerp(a.model.heading, b.model.heading, t);
        interpolated.model.position = Vec4::Lerp(a.model.position, b.model.position, t);
        interpolated.wheelFrontLeft = CarWheel::Lerp(a.wheelFrontLeft, b.wheelFrontLeft, t);
        interpolated.wheelFrontRight = CarWheel::Lerp(a.wheelFrontRight, b.wheelFrontRight, t);
        interpolated.wheelBackLeft = CarWheel::Lerp(a.wheelBackLeft, b.wheelBackLeft, t);
        interpolated.wheelBackRight = CarWheel::Lerp(a.wheelBackRight, b.wheelBackRight, t);

        return interpolated;
    }
};
