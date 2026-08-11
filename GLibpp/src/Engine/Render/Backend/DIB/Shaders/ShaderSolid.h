#pragma once

#include "FragmentShader.h"

namespace GLibpp::Render {

    // Passthrough: vraci zakladni barvu instance beze zmeny.
    // Presne semantika puvodniho drawTriangle(color) - stoji na ni stavajici
    // testy rasterizace (presne hodnoty barev vcetne depth testu).
    struct ShaderSolid {

        struct TriCtx {
            uint32_t color;
        };

        static TriCtx setup(const TriangleInput& tri, const ShaderUniforms&) noexcept
        {
            return { tri.color.toRGBA() };
        }

        static uint32_t shade(const TriCtx& ctx, const PixelInput&, const ShaderUniforms&) noexcept
        {
            return ctx.color;
        }
    };

}
