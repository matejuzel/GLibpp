#pragma once

#include "FragmentShader.h"
#include <cstdint>

namespace GLibpp::Render {

    // Ploche Lambertovo stinovani - vypocet prenesen doslova z byvale sekce
    // "smerove svetlo" v DeviceDIB::rasterizeMesh: smerove svetlo ve view
    // space, intenzita = kAmbient + clamp(N.L, 0, 1) * (kDiffuse - kAmbient).
    //
    // Normala je zatim per trojuhelnik, takze cela prace probehne v setup()
    // a shade() vraci konstantu - per-pixel cena je stejna jako u ShaderSolid.
    // Az Mesh dostane per-vertex normaly, vznikne SmoothLambert, ktery presune
    // vyhodnoceni do shade() nad interpolovanou normalou.
    struct ShaderLambert {

        // smerove svetlo zepredu v prostoru kamery (uz jednotkove)
        static constexpr float kLightX = 0.0f;
        static constexpr float kLightY = 0.0f;
        static constexpr float kLightZ = -1.0f;

        // difuzni rozsah: ambientni podlaha az plna barva
        static constexpr float kAmbient = 0.2f;
        static constexpr float kDiffuse = 1.0f;

        struct TriCtx {
            uint32_t shaded;
        };

        static TriCtx setup(const TriangleInput& tri, const ShaderUniforms&) noexcept
        {
            float dotNL = tri.nx * kLightX + tri.ny * kLightY + tri.nz * kLightZ;
            if (dotNL < 0.0f) dotNL = 0.0f;
            if (dotNL > 1.0f) dotNL = 1.0f;

            const float I = kAmbient + dotNL * (kDiffuse - kAmbient);

            return { Color(
                uint8_t(tri.color.r * I),
                uint8_t(tri.color.g * I),
                uint8_t(tri.color.b * I),
                tri.color.a
            ).toRGBA() };
        }

        static uint32_t shade(const TriCtx& ctx, const PixelInput&, const ShaderUniforms&) noexcept
        {
            return ctx.shaded;
        }
    };

}
