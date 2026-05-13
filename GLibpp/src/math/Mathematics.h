#pragma once
#include <cmath>

namespace GLibpp::Math {

    constexpr double pi = 3.14159265358979323846;

    constexpr float deg2rad(float deg) 
    {
        return deg * static_cast<float>(pi / 180.0);
    }
    constexpr double deg2rad(double deg)
    {
        return deg * pi / 180.0;
    }


    constexpr float rad2deg(float rad) 
    {
        return rad * static_cast < float>(180.0 / pi);
    }
    constexpr double rad2deg(double rad)
    {
        return rad * 180.0 / pi;
    }

};
