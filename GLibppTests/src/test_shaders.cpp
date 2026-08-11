// Fragment shadery: LUT dispatch (fragmentFunction), presne hodnoty
// ShaderSolid/ShaderLambert, prepinani shaderu mezi kresbami, depth test
// a wireframe pres shader cestu.
//
// Uplnost dispatch tabulky vuci FragmentShaderId::Count je overena uz
// v prekladu (static_assert v RasterizerDIB::fragmentFunction).

#define NOMINMAX          // jinak makra min/max z windows.h rozbiji std::min/std::max
#include <windows.h>

#include <algorithm>
#include <string>

// DeviceTargetBase.h neni self-contained (pouziva RenderTargetDescriptor, ale
// neincluduje ho) - v aplikaci to projde diky poradi includu v Renderer.h
#include "RenderTargetDescriptor.h"
#include "RasterizerDIB.h"

#include "TestRunner.h"

using GLibpp::Render::Color;
using GLibpp::Render::DeviceTargetDIB;
using GLibpp::Render::FragmentShaderId;
using GLibpp::Render::PixelInput;
using GLibpp::Render::RasterizerDIB;
using GLibpp::Render::RenderTargetDescriptor;
using GLibpp::Render::ShaderLambert;
using GLibpp::Render::ShaderUniforms;
using GLibpp::Render::TriangleInput;

namespace {

    constexpr int W = 16;
    constexpr int H = 16;

    // trojuhelnik bezpecne pokryvajici pixel (5, 5)
    // (plna agregatni inicializace - Color nema default konstruktor)
    TriangleInput makeTri(const Color& color, float nx, float ny, float nz,
                          float z = 0.0f, bool wireframe = false)
    {
        return TriangleInput{
            2.0f,  2.0f,  z,
            13.0f, 2.0f,  z,
            2.0f,  13.0f, z,
            0.0f, 0.0f, 1.0f,   // UV nulove, invW = 1 (afinni pripad)
            0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,
            nx, ny, nz,
            color,
            wireframe
        };
    }

    void clearColor(DeviceTargetDIB& t, uint32_t c)
    {
        std::fill_n(t.framebuffer, size_t(W) * H, c);
    }

    uint32_t pixelAt(const DeviceTargetDIB& t, int x, int y)
    {
        return t.framebuffer[size_t(y) * W + x];
    }

    // ocekavana hodnota Lambertu - stejna aritmetika jako ShaderLambert::setup
    uint32_t expectedLambert(const Color& base, float I)
    {
        return Color(
            uint8_t(base.r * I),
            uint8_t(base.g * I),
            uint8_t(base.b * I),
            base.a
        ).toRGBA();
    }

}

namespace GLibppTests {

    void runShaderTests()
    {
        section("Fragment shadery (LUT dispatch + Solid/Lambert)");

        try
        {
            DeviceTargetDIB target(RenderTargetDescriptor::FramebufferRGBA32bit(W, H));
            DeviceTargetDIB depth(RenderTargetDescriptor::Depthbuffer32F(W, H));

            const Color base(10, 200, 60, 255);
            const ShaderUniforms uni{ 1.0f / float(W), 1.0f / float(H) };

            const auto solidFn = RasterizerDIB::fragmentFunction(FragmentShaderId::Solid);
            const auto lambertFn = RasterizerDIB::fragmentFunction(FragmentShaderId::Lambert);
            const auto uvFn = RasterizerDIB::fragmentFunction(FragmentShaderId::UvDebug);

            // --- Solid: presna zakladni barva ---
            clearColor(target, 0u);
            solidFn(target, nullptr, makeTri(base, 1.0f, 0.0f, 0.0f), uni);
            check(pixelAt(target, 5, 5) == base.toRGBA(),
                "Solid vraci presne zakladni barvu instance");

            // --- Lambert: normala proti svetlu (0,0,-1) -> plna intenzita ---
            clearColor(target, 0u);
            lambertFn(target, nullptr, makeTri(base, 0.0f, 0.0f, -1.0f), uni);
            check(pixelAt(target, 5, 5) == expectedLambert(base, 1.0f),
                "Lambert s normalou proti svetlu vraci plnou barvu (I = 1.0)");

            // --- Lambert: normala kolmo na svetlo -> ambientni podlaha ---
            clearColor(target, 0u);
            lambertFn(target, nullptr, makeTri(base, 1.0f, 0.0f, 0.0f), uni);
            check(pixelAt(target, 5, 5) == expectedLambert(base, ShaderLambert::kAmbient),
                "Lambert s normalou kolmo na svetlo vraci ambientni podlahu (I = 0.2)");

            // --- UvDebug: normalizovane souradnice odvozene z uniformu ---
            clearColor(target, 0u);
            uvFn(target, nullptr, makeTri(base, 0.0f, 0.0f, -1.0f), uni);
            {
                // stejna aritmetika jako ShaderUvDebug::shade (stred pixelu / rozliseni)
                const float u = (5.0f + 0.5f) / float(W);
                const float v = (5.0f + 0.5f) / float(H);
                const uint32_t expected = Color(uint8_t(u * 255.0f), uint8_t(v * 255.0f), 0, 255).toRGBA();
                check(pixelAt(target, 5, 5) == expected,
                    "UvDebug: pixel (5,5) ma barvu odvozenou z u/v (uniformy dotekly do shade)");

                const float u3 = (3.0f + 0.5f) / float(W);
                const float v8 = (8.0f + 0.5f) / float(H);
                const uint32_t expected38 = Color(uint8_t(u3 * 255.0f), uint8_t(v8 * 255.0f), 0, 255).toRGBA();
                check(pixelAt(target, 3, 8) == expected38,
                    "UvDebug: u/v se meni po pixelech (skutecne per-pixel shade)");
            }

            // --- prepnuti shaderu meni vystup nad stejnym vstupem ---
            const TriangleInput sameTri = makeTri(base, 1.0f, 0.0f, 0.0f);

            clearColor(target, 0u);
            solidFn(target, nullptr, sameTri, uni);
            const uint32_t bySolid = pixelAt(target, 5, 5);

            clearColor(target, 0u);
            lambertFn(target, nullptr, sameTri, uni);
            const uint32_t byLambert = pixelAt(target, 5, 5);

            check(bySolid != byLambert,
                "prepnuti FragmentShaderId meni vysledek nad stejnym trojuhelnikem");

            // --- depth test funguje i pres shader cestu ---
            clearColor(target, 0u);
            std::fill_n(depth.depthbuffer.data(), depth.depthbuffer.size(), 1.0f);

            const Color nearColor(0, 255, 0, 255);
            const Color farColor(255, 0, 0, 255);

            solidFn(target, &depth, makeTri(farColor, 0.0f, 0.0f, -1.0f, 0.5f), uni);
            solidFn(target, &depth, makeTri(nearColor, 0.0f, 0.0f, -1.0f, -0.5f), uni);
            check(pixelAt(target, 5, 5) == nearColor.toRGBA(),
                "depth pres shader cestu: blizsi kresleny druhy prekresli vzdalenejsi");

            clearColor(target, 0u);
            std::fill_n(depth.depthbuffer.data(), depth.depthbuffer.size(), 1.0f);

            solidFn(target, &depth, makeTri(nearColor, 0.0f, 0.0f, -1.0f, -0.5f), uni);
            solidFn(target, &depth, makeTri(farColor, 0.0f, 0.0f, -1.0f, 0.5f), uni);
            check(pixelAt(target, 5, 5) == nearColor.toRGBA(),
                "depth pres shader cestu: vzdalenejsi kresleny druhy blizsi neprepise");

            // --- wireframe bere barvu ze shaderu (stinovany dratovy model) ---
            clearColor(target, 0u);
            lambertFn(target, nullptr, makeTri(base, 1.0f, 0.0f, 0.0f, 0.0f, true), uni);
            check(pixelAt(target, 5, 2) == expectedLambert(base, ShaderLambert::kAmbient),
                "wireframe obrys ma barvu ze shade() (Lambert, ne zakladni barvu)");
            check(pixelAt(target, 5, 5) == 0u,
                "wireframe nevyplnuje vnitrek trojuhelniku");
        }
        catch (const std::exception& e)
        {
            check(false, std::string("vyjimka pri tvorbe targetu: ") + e.what());
        }
    }

}
