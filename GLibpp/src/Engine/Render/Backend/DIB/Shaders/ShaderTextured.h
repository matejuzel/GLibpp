#pragma once

#include "FragmentShader.h"
#include <cmath>
#include <cstdint>

namespace GLibpp::Render {

    // Texturovany povrch: nearest vzorkovani bindnute textury (SetTexture)
    // na perspektivne korektnich UV - rasterizer interpoluje u/w, v/w, 1/w,
    // deleni probiha tady per pixel. UV mimo [0,1] se wrapuje (repeat).
    // Bez bindnute textury (nebo u degenerovaneho invW) vraci zakladni barvu
    // instance - stejne "fallback" chovani jako ShaderSolid.
    //
    // Dvirka: bilinearni filtr a modulace osvetlenim (x Lambert intenzita)
    // jsou lokalni upravy shade() - nic jineho se menit nemusi.
    struct ShaderTextured {

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

            // wrap (repeat) do [0, 1) bez std::floor - v Debugu (/Od) je floor
            // CRT volani a per pixel je citelne drahe; int-cast + korekce
            // znamenka je korektni pro libovolne u
            u -= float(int(u)); if (u < 0.0f) u += 1.0f;
            v -= float(int(v)); if (v < 0.0f) v += 1.0f;

            uint32_t tx = uint32_t(u * float(uni.textureWidth));
            uint32_t ty = uint32_t(v * float(uni.textureHeight));
            if (tx >= uni.textureWidth)  tx = uni.textureWidth - 1;
            if (ty >= uni.textureHeight) ty = uni.textureHeight - 1;

            return uni.texture[size_t(ty) * uni.textureWidth + tx];
        }
    };

}
