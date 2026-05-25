#pragma once

#include "BicycleParams.h"
#include "Vec4.h"
#include "Quaternion.h"
#include "Mathematics.h"

namespace GLibpp::Physics {

    class BicycleModel {

    public:
        BicycleParams params;

        Vec4 position;        // svìtová pozice
        Quaternion heading;   // NEW – plná 3D orientace

    private:
        

        float speed = 0.0f;
        float steerAngle = 0.0f;

    public:

        BicycleModel(float wheelRadius, float wheelBase, float wheelTrack, float maxSteerAngle)
            : params(wheelRadius, wheelBase, wheelTrack, maxSteerAngle)
            , heading(Quaternion::Identity())
            , position(0.0f, wheelRadius, 0.0f)
        {
        }

        static BicycleModel Basic()
        {
            return BicycleModel(0.6f, 3.0f, 3.0f, Math::deg2rad(35.0f));
        }

        float getIcr() const
        {
            return params.wheelBase / tan(steerAngle);
        }

        float getIcrBL() const
        {
            float R = getIcr();
            return fabs(R + params.wheelTrack * 0.5f);
        }

        float getIcrBR() const
        {
            float R = getIcr();
            return fabs(R - params.wheelTrack * 0.5f);
        }

        float getIcrFL() const
        {
            float R_BL = getIcrBL();
            return sqrtf(R_BL * R_BL + params.wheelBase * params.wheelBase);
        }

        float getIcrFR() const
        {
            float R_BR = getIcrBR();
            return sqrtf(R_BR * R_BR + params.wheelBase * params.wheelBase);
        }

        float getSteerAngleLeft() const
        {
            return atan(params.wheelBase / (getIcr() + params.wheelTrack * 0.5f));
        }

        float getSteerAngleRight() const
        {
            return atan(params.wheelBase / (getIcr() - params.wheelTrack * 0.5f));
        }


        void update(float dt)
        {
            Vec4 forward = heading * Vec4(0, 0, 1, 0);
            Vec4 right = heading * Vec4(1, 0, 0, 0);

            // rovná jízda
            if (fabs(steerAngle) < 0.0001f) {
                position = position + (forward * (speed * dt));
                return;
            }

            float R = getIcr();
            float dtheta = (speed * dt) / R;

            // ICR leží vpravo od zadní nápravy
            Vec4 icr = position + right * R;

            // rotace kolem Y
            Quaternion delta = Quaternion::FromAxisAngle(Vec4(0, 1, 0, 0), dtheta);

            // otoèíme heading
            heading = (delta * heading).normalized();

            // otoèíme pozici kolem ICR
            Vec4 rel = position - icr;
            rel = delta * rel;
            position = icr + rel;
        }

        void accelerate(float dv)
        {
            speed += dv;
            if (fabs(speed) < 0.01f)
                speed = 0.0f;
        }

        void brake(float faktor)
        {
            if (speed > 0.1f) speed -= faktor;
            if (speed < -0.1f) speed += faktor;
        }

        void steer(float dAngle)
        {
            steerAngle += dAngle;

            if (steerAngle > params.maxSteerAngle)
                steerAngle = params.maxSteerAngle;

            if (steerAngle < -params.maxSteerAngle)
                steerAngle = -params.maxSteerAngle;
        }

        void steerReset(float dt)
        {
            float delta = 4.0f * params.maxSteerAngle * dt;

            if (fabs(steerAngle) <= delta)
                steerAngle = 0.0f;
            else
                steerAngle -= copysign(delta, steerAngle);
        }

        float getSpeed() const { return speed; }

        Quaternion getHeading() const { return heading; }

        Vec4 getPosition() const { return position; }

        Mtx4 getTransformation() const
        {
            return Mtx4::Identity().translate(getPosition().x, getPosition().y, getPosition().z) * getHeading().toMatrix();
        }

    };
}