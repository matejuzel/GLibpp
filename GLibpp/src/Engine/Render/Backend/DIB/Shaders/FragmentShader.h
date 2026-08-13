#pragma once

#include "Color.h"
#include <bit>
#include <concepts>
#include <cstdint>

namespace GLibpp::Render {

    // ---- fragment shadery DIB backendu ----
    //
    // Kazdy shader je BEZSTAVOVA struct se dvema statickymi funkcemi a vlastnim
    // typem TriCtx ("varyings" pripravene jednou na trojuhelnik):
    //
    //     struct MujShader {
    //         struct TriCtx { ... };
    //         static TriCtx   setup(const TriangleInput& tri, const ShaderUniforms& uni) noexcept;               // 1x na trojuhelnik
    //         static uint32_t shade(const TriCtx& ctx, const PixelInput& px, const ShaderUniforms& uni) noexcept; // 1x na pixel, vraci ARGB
    //     };
    //
    // Zadne dedeni ani CRTP - kontrakt vynucuje koncept FragmentShader nize.
    //
    // Dispatch: RasterizerDIB::fragmentFunction(id) vraci z constexpr tabulky
    // ukazatel na instanciaci rasterizeTriangleT<Shader>. Nepr imy call je tedy
    // JEDEN na trojuhelnik (fetch tabulky dokonce jen jednou na draw) a shade()
    // se do vnitrni smycky inlinuje (Release) - stejny princip jako specializace
    // smycky u produkcnich SW rasterizeru (llvmpipe, SwiftShader), jen staticky.
    // Per-pixel ukazatel na funkci by inlining zabil, proto tudy ne.
    //
    // Pridani shaderu = novy header v teto slozce + radek ve FragmentShaderId
    // + radek v kFragmentDispatch (poradi hlida static_assert na Count - stejny
    // vzor jako dispatch tabulka DrawCommandu v Rendereru).
    //
    // Otevrena dvirka (vedome evoluce kontraktu):
    //  - per-vertex normaly a dalsi varyingy: TriangleInput je struct, pridani
    //    poli nic nerozbiji; barycentricke gradienty patri do setup()
    //    (hranove funkce e0/e1/e2 ve smycce uz JSOU barycentricke vahy)
    //  - dalsi uniformy (cas, svetlo, textury): pridani pole do ShaderUniforms
    //    je nerozbijeci zmena - struct uz tece do setup i shade
    //  - discard/alpha test: navratovy typ shade() -> maly struct {color, discard}
    //    (pozor pak na early-z: hloubka se dnes zapisuje pred shade)

    // Per-draw konstanty spolecne vsem shaderum (obdoba GPU uniformu) - plni je
    // rasterizeMesh z kontextu, shader je NEVLASTNI (zustava bezstavovy kod).
    //
    // Normalizovane souradnice si shader odvozuje sam - pixel je primitiv
    // (presna celociselna identita, viz gl_FragCoord), konvence normalizace
    // je volba shaderu (ShaderToy idiom):
    //     float u01 = (float(px.x) + 0.5f) * uni.invWidth;   // [0, 1]
    //     float uS  = u01 * 2.0f - 1.0f;                     // [-1, 1]
    struct ShaderUniforms {
        float invWidth;  // 1 / sirka viewportu v pixelech
        float invHeight; // 1 / vyska viewportu v pixelech

        // bindnuta textura pruchodu (command SetTexture); nullptr = zadna.
        // ARGB 0xAARRGGBB, radky shora dolu - resolvuje rasterizeMesh z residency
        const uint32_t* texture;
        uint32_t textureWidth;
        uint32_t textureHeight;

        // Filtracni urovne (RIP-map, viz RipMapDIB.h): tabulka offsetu do
        // texture[] indexovana [j * levelsU + i], kde i puli sirku a j vysku.
        // Textura bez urovni (dynamicka - plni ji rendering) ma levelsU ==
        // levelsV == 1 a jediny nulovy offset, takze sampler nevetvi.
        // INVARIANT: levelOffsets != nullptr  =>  levelsU >= 1 && levelsV >= 1
        const uint32_t* levelOffsets;
        uint32_t levelsU;
        uint32_t levelsV;
    };

    // ---- pomocna matematika samplovani (spolecna vic shaderum) ----

    // Linearni interpolace dvou ARGB texelu 8bitovou vahou (w8: 0 = a, 256 = b).
    // Dva kanaly naraz v 16bitovych pruzich jednoho registru: (a >> 8) drzi
    // A a G, (a & 0x00FF00FF) drzi R a B. Nepretece do souseda: kazdy pruh je
    // nejvys 255 * 256 = 65280 < 65536.
    inline uint32_t lerpARGB(uint32_t a, uint32_t b, uint32_t w8) noexcept
    {
        const uint32_t iw = 256u - w8;

        const uint32_t agA = (a >> 8) & 0x00FF00FFu, rbA = a & 0x00FF00FFu;
        const uint32_t agB = (b >> 8) & 0x00FF00FFu, rbB = b & 0x00FF00FFu;

        const uint32_t ag = ((agA * iw + agB * w8) >> 8) & 0x00FF00FFu;
        const uint32_t rb = ((rbA * iw + rbB * w8) >> 8) & 0x00FF00FFu;

        return (ag << 8) | rb;
    }

    // Bilinearni vzorek urovne (u, v uz zwrapovane do [0,1)). Stred texelu lezi
    // na (i + 0.5), proto posun o -0.5; okraje se wrapuji (repeat), takze
    // sousedni texel pres hranu se bere z protejsi strany.
    inline uint32_t bilinearSampleARGB(const uint32_t* level, uint32_t levelWidth, uint32_t levelHeight,
                                       float u, float v) noexcept
    {
        const float fx = u * float(levelWidth) - 0.5f;
        const float fy = v * float(levelHeight) - 0.5f;

        // floor bez std::floor (v /Od je to CRT volani per pixel): int-cast
        // truncuje k nule, pro zaporny vstup je nutna korekce o jednicku
        int ix = int(fx); if (fx < float(ix)) --ix;
        int iy = int(fy); if (fy < float(iy)) --iy;

        const uint32_t wu = uint32_t((fx - float(ix)) * 256.0f);
        const uint32_t wv = uint32_t((fy - float(iy)) * 256.0f);

        // fx muze byt az -0.5, takze ix == -1 -> wrap na posledni texel
        uint32_t x0 = uint32_t(ix < 0 ? ix + int(levelWidth) : ix);
        uint32_t y0 = uint32_t(iy < 0 ? iy + int(levelHeight) : iy);
        if (x0 >= levelWidth)  x0 = 0;
        if (y0 >= levelHeight) y0 = 0;

        uint32_t x1 = x0 + 1; if (x1 >= levelWidth)  x1 = 0;
        uint32_t y1 = y0 + 1; if (y1 >= levelHeight) y1 = 0;

        const uint32_t* row0 = level + size_t(y0) * levelWidth;
        const uint32_t* row1 = level + size_t(y1) * levelWidth;

        return lerpARGB(lerpARGB(row0[x0], row0[x1], wu),
                        lerpARGB(row1[x0], row1[x1], wu), wv);
    }

    // floor(log2(|x|)) z exponentu floatu, clampnute do [0, maxLevel].
    // Vyber filtracni urovne presnost nepotrebuje a log2 je v /Od CRT volani
    // per pixel; x <= 0 i denormal daji 0, NaN/inf spadne na maxLevel.
    inline uint32_t log2FloorClamped(float x, uint32_t maxLevel) noexcept
    {
        const int32_t e = int32_t((std::bit_cast<uint32_t>(x) >> 23) & 0xFFu) - 127;
        if (e <= 0) return 0;
        return (uint32_t(e) < maxLevel) ? uint32_t(e) : maxLevel;
    }

    // max(|a|, |b|) bez fabsf (v /Od volani) - obalka stopy pixelu v jedne ose
    inline float absMax(float a, float b) noexcept
    {
        if (a < 0.0f) a = -a;
        if (b < 0.0f) b = -b;
        return a > b ? a : b;
    }

    // Gradient afinniho atributu v obrazovkovem prostoru: pro hodnoty a0, a1, a2
    // ve vrcholech resi soustavu 2x2 (stejny vzorec jako planeGrad v rasterizeru,
    // ktery si jim krokuje roviny). Delta hrany a 1/det si volajici spocita
    // jednou pro vsechny atributy. Nezavisle na vinuti - det i citatele meni
    // znamenko spolecne.
    struct ScreenGradient { float dx, dy; };

    inline ScreenGradient screenGradient(float a0, float a1, float a2,
                                        float ex1, float ey1, float ex2, float ey2,
                                        float invArea2) noexcept
    {
        const float da1 = a1 - a0;
        const float da2 = a2 - a0;
        return { (da1 * ey2 - da2 * ey1) * invArea2,
                 (da2 * ex1 - da1 * ex2) * invArea2 };
    }

    // per-triangle vstup: screen-space vrcholy + atributy
    struct TriangleInput {
        float x0, y0, z0;   // screen x, y + NDC z (po perspektivnim deleni)
        float x1, y1, z1;
        float x2, y2, z2;

        // texturovaci UV + prevracene clip-space w per vrchol; UV se interpoluji
        // perspektivne korektne (rasterizer interpoluje u/w, v/w, 1/w - shader
        // deli az per pixel). Mesh bez UV dodava nuly, invW pro platne vrcholy
        // je vzdy > 0 (frustum test garantuje w >= nearZ)
        float u0, v0, invW0;
        float u1, v1, invW1;
        float u2, v2, invW2;

        // plocha normala trojuhelniku ve view space (dodava ji geometricka
        // faze v rasterizeMesh; per-vertex normaly prijdou az s atributem v Mesh)
        float nx, ny, nz;

        Color color;        // zakladni barva instance
        bool wireframe;     // obrys misto vyplne (barvu bere ze shade() v centroidu)
    };

    // per-pixel vstup
    struct PixelInput {
        int x;
        int y;
        float z;            // interpolovana NDC hloubka pixelu

        // perspektivne korektni atributy: interpolovane u/w, v/w a 1/w -
        // skutecne UV si shader spocita delenim (plati jen kdyz to potrebuje):
        //     float w = 1.0f / px.invW;
        //     float u = px.uOverW * w, v = px.vOverW * w;
        float uOverW;
        float vOverW;
        float invW;
    };

    template <typename S>
    concept FragmentShader = requires(const TriangleInput& tri, const typename S::TriCtx& ctx,
                                      const PixelInput& px, const ShaderUniforms& uni)
    {
        { S::setup(tri, uni) } noexcept -> std::same_as<typename S::TriCtx>;
        { S::shade(ctx, px, uni) } noexcept -> std::convertible_to<uint32_t>;
    };

}
