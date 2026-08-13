#pragma once

#include "FragmentShader.h"
#include <cstdint>

namespace GLibpp::Render {

    // Texturovany povrch s bilinearni filtraci: ctyri sousedni texely urovne 0
    // vazene podle zlomkove casti UV (viz bilinearSampleARGB). Proti nearest
    // varante (ShaderTextured) zmizi kostickovani pri zvetseni a zmirni se
    // sum pri zmenseni - ten uplne resi az vyber filtracni urovne
    // (ShaderTexturedAniso).
    //
    // Zamerne NEPOCITA gradienty UV: pro dynamicke textury (capture panely,
    // ktere plni rendering) zadne urovne neexistuji, takze by to byla prace
    // navic bez efektu. Cena je 4 tapy + 3 lerpy na pixel.
    struct ShaderTexturedBilinear {

        struct TriCtx {
            uint32_t fallback;
        };

        static TriCtx setup(const TriangleInput& tri, const ShaderUniforms&) noexcept
        {
            return { tri.color.toRGBA() };
        }

        static uint32_t shade(const TriCtx& ctx, const PixelInput& px, const ShaderUniforms& uni) noexcept
        {
            if (uni.texture == nullptr || px.invW <= 0.0f)
                return ctx.fallback;

            // perspektivni korekce
            const float w = 1.0f / px.invW;
            float u = px.uOverW * w;
            float v = px.vOverW * w;

            // wrap (repeat) do [0, 1) bez std::floor - viz ShaderTextured
            u -= float(int(u)); if (u < 0.0f) u += 1.0f;
            v -= float(int(v)); if (v < 0.0f) v += 1.0f;

            return bilinearSampleARGB(uni.texture, uni.textureWidth, uni.textureHeight, u, v);
        }
    };

}
