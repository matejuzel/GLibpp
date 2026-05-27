#pragma once
#include <cmath>
#include <iostream>
#include <bit>

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

    inline float reciprocal(float x)
    {
        // rychlý odhad pomocí bit hacku
		float y = std::bit_cast<float>(0x7EF311C2 - std::bit_cast<uint32_t>(x));

        // dvì iterace NR
        y = y * (2.0f - x * y);
        y = y * (2.0f - x * y);

        return y;
    }

    inline float reciprocal_debug(float x, int steps, int magic = 0x7EF311C2)
    {

        uint32_t ix = std::bit_cast<uint32_t>(x);
        uint32_t iy = magic - ix;

        float y = std::bit_cast<float>(iy);

        std::cout << "initial guess: " << y
            << " (err = " << fabsf(y - 1.0f / x) << ")\n";

        for (int i = 0; i < steps; ++i) {
            y = y * (2.0f - x * y);
            std::cout << "iteration " << i << ": " << y
                << " (err = " << fabsf(y - 1.0f / x) << ")\n";
        }

        return y;
    }


    inline float abs(float x) {
        return std::abs(x);
    }

    inline float fastDiv(float numerator, float denominator)
    {
        return numerator * reciprocal(denominator);
	}

};
