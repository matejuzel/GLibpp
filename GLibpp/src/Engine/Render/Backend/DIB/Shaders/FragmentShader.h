#pragma once

#include "Color.h"
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
    };

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
