#pragma once

#include "FragmentShader.h"
#include "RipMapDIB.h"
#include <cstdint>

namespace GLibpp::Render {

    // Texturovany povrch s anizotropni filtraci pres RIP-map urovne.
    //
    // Klic je stopa pixelu v texturovem prostoru - jak velky kus textury
    // pripada na jeden pixel. Potrebuje derivace UV podle obrazovky, ktere
    // GPU pocita konecnymi diferencemi pres 2x2 quad (shader je pro hardware
    // cerna skrinka). U nas quad potreba NENI: rasterizer UV interpoluje
    // rovinami, takze gradienty afinnich atributu u/w, v/w a 1/w se daji
    // spocitat presne - jednou na trojuhelnik v setup(). Pro u = A / C
    // (A = u/w, C = 1/w) plati
    //
    //     du/dx = w * (A'x - u * C'x)
    //
    // tedy per pixel jen odecteni a dve nasobeni na kazdou derivaci. Exaktne,
    // bez plytvani na maskovanych pixelech, ktere quady na hranach maji.
    //
    // Z derivaci se vezme obalka stopy NEZAVISLE v u a v; kazda osa si vybere
    // svou uroven pulení (i pro sirku, j pro vysku). Tim se drzi detail ve
    // smeru, kde je stopa uzka - presne to, co izotropni mip zahazuje.
    // Uvnitr vybrane urovne se vzorkuje bilinearne, takze jde o jeden lookup,
    // ne o N tapu podel delsi osy jako u GPU anizotropie.
    //
    // Textura bez urovni (dynamicka) ma levelsU == levelsV == 1, takze vyber
    // vzdy padne na uroven 0 a shader se zvrhne v bilinearni vzorkovani.
    struct ShaderTexturedAniso {

        struct TriCtx {
            uint32_t fallback;

            // gradienty afinnich atributu v obrazovkovem prostoru
            float aGx, aGy; // u/w
            float bGx, bGy; // v/w
            float cGx, cGy; // 1/w
        };

        static TriCtx setup(const TriangleInput& tri, const ShaderUniforms&) noexcept
        {
            TriCtx ctx{ tri.color.toRGBA(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

            const float ex1 = tri.x1 - tri.x0, ey1 = tri.y1 - tri.y0;
            const float ex2 = tri.x2 - tri.x0, ey2 = tri.y2 - tri.y0;

            // dvojnasobek plochy; degenerovany trojuhelnik nechava gradienty
            // nulove -> stopa 0 -> uroven 0 (rasterizer ho stejne zahodi)
            const float area2 = ex1 * ey2 - ex2 * ey1;
            if (area2 > -1e-6f && area2 < 1e-6f) return ctx;

            const float inv = 1.0f / area2;

            // tytez veliciny, ktere rasterizer interpoluje rovinami
            const ScreenGradient a = screenGradient(tri.u0 * tri.invW0, tri.u1 * tri.invW1,
                                                    tri.u2 * tri.invW2, ex1, ey1, ex2, ey2, inv);
            const ScreenGradient b = screenGradient(tri.v0 * tri.invW0, tri.v1 * tri.invW1,
                                                    tri.v2 * tri.invW2, ex1, ey1, ex2, ey2, inv);
            const ScreenGradient c = screenGradient(tri.invW0, tri.invW1, tri.invW2,
                                                    ex1, ey1, ex2, ey2, inv);

            ctx.aGx = a.dx; ctx.aGy = a.dy;
            ctx.bGx = b.dx; ctx.bGy = b.dy;
            ctx.cGx = c.dx; ctx.cGy = c.dy;
            return ctx;
        }

        static uint32_t shade(const TriCtx& ctx, const PixelInput& px, const ShaderUniforms& uni) noexcept
        {
            if (uni.texture == nullptr || uni.levelOffsets == nullptr || px.invW <= 0.0f)
                return ctx.fallback;

            // perspektivni korekce
            const float w = 1.0f / px.invW;
            float u = px.uOverW * w;
            float v = px.vOverW * w;

            // derivace skutecnych UV podle obrazovky (viz komentar u struktury)
            const float dudx = w * (ctx.aGx - u * ctx.cGx);
            const float dudy = w * (ctx.aGy - u * ctx.cGy);
            const float dvdx = w * (ctx.bGx - v * ctx.cGx);
            const float dvdy = w * (ctx.bGy - v * ctx.cGy);

            // obalka stopy pixelu v texelech, kazda osa zvlast
            const float footprintU = absMax(dudx, dudy) * float(uni.textureWidth);
            const float footprintV = absMax(dvdx, dvdy) * float(uni.textureHeight);

            const uint32_t i = log2FloorClamped(footprintU, uni.levelsU - 1);
            const uint32_t j = log2FloorClamped(footprintV, uni.levelsV - 1);

            // wrap (repeat) do [0, 1) bez std::floor - viz ShaderTextured
            u -= float(int(u)); if (u < 0.0f) u += 1.0f;
            v -= float(int(v)); if (v < 0.0f) v += 1.0f;

            const uint32_t* level = uni.texture + uni.levelOffsets[size_t(j) * uni.levelsU + i];

            return bilinearSampleARGB(level,
                                      ripLevelSize(uni.textureWidth, i),
                                      ripLevelSize(uni.textureHeight, j),
                                      u, v);
        }
    };

}
