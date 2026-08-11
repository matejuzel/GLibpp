// Rasterizace: exclusive pokryti (top-left fill rule), subpixel presnost, depth test.
//
// Tenhle soubor hlida invariant, na kterem stoji fixed-point rasterizer:
// pixel na hrane sdilene dvema trojuhelniky patri PRESNE JEDNOMU z nich.
// Kdyby se rasterizer vratil k float hranovym funkcim nebo k celociselnym
// souradnicim vrcholu, testy nize spadnou (viz CLAUDE.md).

#define NOMINMAX          // jinak makra min/max z windows.h rozbiji std::min/std::max
#include <windows.h>

#include <cmath>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <string>

// DeviceTargetBase.h neni self-contained (pouziva RenderTargetDescriptor, ale
// neincluduje ho) - v aplikaci to projde diky poradi includu v Renderer.h
#include "RenderTargetDescriptor.h"
#include "RasterizerDIB.h"

#include "TestRunner.h"

using GLibpp::Render::RasterizerDIB;
using GLibpp::Render::DeviceTargetDIB;
using GLibpp::Render::RenderTargetDescriptor;

namespace {

    constexpr int W = 40;
    constexpr int H = 40;

    struct Tri { float x0, y0, z0, x1, y1, z1, x2, y2, z2; };

    void clearColor(DeviceTargetDIB& t, uint32_t c)
    {
        std::fill_n(t.framebuffer, size_t(W) * H, c);
    }

    // vykresli jeden trojuhelnik do cisteho targetu a vrati masku zasazenych pixelu
    std::vector<uint8_t> coverage(DeviceTargetDIB& t, const Tri& tr)
    {
        clearColor(t, 0u);
        RasterizerDIB::drawTriangle(t, nullptr,
            tr.x0, tr.y0, tr.z0, tr.x1, tr.y1, tr.z1, tr.x2, tr.y2, tr.z2,
            0xFFFFFFFF, false);

        std::vector<uint8_t> m(size_t(W) * H, 0);
        for (size_t i = 0; i < m.size(); ++i) m[i] = t.framebuffer[i] ? 1 : 0;
        return m;
    }

    // Ctverec [x0,x1) x [y0,y1) rozdeleny diagonalou na dva trojuhelniky musi
    // pokryt PRESNE ty pixely, jejichz stred lezi ve ctverci - zadny dvakrat,
    // zadny vynechany. Ocekavany rozsah: px od ceil(x0-0.5) do ceil(x1-0.5)-1
    // (leva/horni hrana patri dovnitr, prava/dolni ne = top-left fill rule).
    void testQuadTiling(DeviceTargetDIB& t, float x0, float y0, float x1, float y1, const std::string& label)
    {
        const Tri a{ x0, y0, 0.0f,  x1, y0, 0.0f,  x1, y1, 0.0f };
        const Tri b{ x0, y0, 0.0f,  x1, y1, 0.0f,  x0, y1, 0.0f };

        const std::vector<uint8_t> ma = coverage(t, a);
        const std::vector<uint8_t> mb = coverage(t, b);

        const int pxLo = int(std::ceil(x0 - 0.5f));
        const int pxHi = int(std::ceil(x1 - 0.5f)) - 1;
        const int pyLo = int(std::ceil(y0 - 0.5f));
        const int pyHi = int(std::ceil(y1 - 0.5f)) - 1;

        int both = 0, missing = 0, outside = 0, covered = 0;

        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
            {
                const size_t i = size_t(y) * W + x;
                const bool inA = ma[i] != 0;
                const bool inB = mb[i] != 0;
                const bool expected = (x >= pxLo && x <= pxHi && y >= pyLo && y <= pyHi);

                if (inA && inB) ++both;                  // dvojite pokryti
                if (expected && !inA && !inB) ++missing; // mezera
                if (!expected && (inA || inB)) ++outside;
                if (inA || inB) ++covered;
            }

        const int expectedCount = (pxHi - pxLo + 1) * (pyHi - pyLo + 1);

        GLibppTests::check(both == 0, label + ": zadny pixel dvakrat (nalezeno " + std::to_string(both) + ")");
        GLibppTests::check(missing == 0, label + ": zadna mezera (nalezeno " + std::to_string(missing) + ")");
        GLibppTests::check(outside == 0, label + ": nic mimo ctverec (nalezeno " + std::to_string(outside) + ")");
        GLibppTests::check(covered == expectedCount,
            label + ": pokryto " + std::to_string(covered) + " pixelu, ocekavano " + std::to_string(expectedCount));
    }

    // Vejir trojuhelniku kolem stredu (jako vicko valce) - vsechny sdilene hrany
    // pod libovolnymi uhly, nic se nesmi kreslit dvakrat.
    void testFanNoOverlap(DeviceTargetDIB& t)
    {
        const int segments = 12;
        const float cx = 20.0f, cy = 20.0f, r = 15.0f;

        std::vector<int> hits(size_t(W) * H, 0);

        for (int s = 0; s < segments; ++s)
        {
            const float a0 = 6.2831853f * float(s) / float(segments);
            const float a1 = 6.2831853f * float(s + 1) / float(segments);

            const Tri tr{
                cx, cy, 0.0f,
                cx + r * std::cos(a0), cy + r * std::sin(a0), 0.0f,
                cx + r * std::cos(a1), cy + r * std::sin(a1), 0.0f
            };

            const std::vector<uint8_t> m = coverage(t, tr);
            for (size_t i = 0; i < hits.size(); ++i) hits[i] += m[i];
        }

        int doubled = 0, total = 0;
        for (int h : hits) { if (h > 1) ++doubled; if (h > 0) ++total; }

        GLibppTests::check(doubled == 0,
            "vejir " + std::to_string(segments) + " segmentu: zadny pixel dvakrat (nalezeno "
            + std::to_string(doubled) + ", pokryto " + std::to_string(total) + ")");
        GLibppTests::check(total > 500, "vejir: pokryti je neprazdne a rozumne velke");
    }

    // Subpixelovy posun musi menit pokryti - kdyby se vrcholy zaokrouhlovaly na
    // cele pixely, oba posuny by daly totozny vysledek.
    void testSubpixelShift(DeviceTargetDIB& t)
    {
        const Tri base{ 5.0f, 5.0f, 0.0f, 25.0f, 5.0f, 0.0f, 5.0f, 25.0f, 0.0f };
        const Tri shifted{ 5.3f, 5.0f, 0.0f, 25.3f, 5.0f, 0.0f, 5.3f, 25.0f, 0.0f };

        const std::vector<uint8_t> mb = coverage(t, base);
        const std::vector<uint8_t> ms = coverage(t, shifted);

        GLibppTests::check(mb != ms,
            "subpixelovy posun 0,3 px meni pokryti (vrcholy nejsou snapnute na cele pixely)");
    }

    // Depth test: blizsi trojuhelnik vyhrava nezavisle na poradi kresleni.
    void testDepthOrder(DeviceTargetDIB& t, DeviceTargetDIB& depth)
    {
        const Tri nearTri{ 5.0f, 5.0f, -0.5f, 30.0f, 5.0f, -0.5f, 5.0f, 30.0f, -0.5f };
        const Tri farTri { 5.0f, 5.0f,  0.5f, 30.0f, 5.0f,  0.5f, 5.0f, 30.0f,  0.5f };

        const uint32_t nearColor = 0xFF00FF00;
        const uint32_t farColor = 0xFFFF0000;

        auto drawPair = [&](const Tri& first, uint32_t c1, const Tri& second, uint32_t c2) -> uint32_t
            {
                clearColor(t, 0u);
                std::fill_n(depth.depthbuffer.data(), depth.depthbuffer.size(), 1.0f);

                RasterizerDIB::drawTriangle(t, &depth, first.x0, first.y0, first.z0,
                    first.x1, first.y1, first.z1, first.x2, first.y2, first.z2, c1, false);
                RasterizerDIB::drawTriangle(t, &depth, second.x0, second.y0, second.z0,
                    second.x1, second.y1, second.z1, second.x2, second.y2, second.z2, c2, false);

                return t.framebuffer[size_t(10) * W + 10]; // bod uvnitr obou
            };

        GLibppTests::check(drawPair(nearTri, nearColor, farTri, farColor) == nearColor,
            "depth: blizsi prvni, vzdalenejsi druhy -> zustane blizsi");
        GLibppTests::check(drawPair(farTri, farColor, nearTri, nearColor) == nearColor,
            "depth: vzdalenejsi prvni, blizsi druhy -> prekresli ho blizsi");
    }

}

namespace GLibppTests {

    void runRasterizerTests()
    {
        section("Rasterizace (RasterizerDIB)");

        try
        {
            DeviceTargetDIB target(RenderTargetDescriptor::FramebufferRGBA32bit(W, H));
            DeviceTargetDIB depth(RenderTargetDescriptor::Depthbuffer32F(W, H));

            check(depth.isDepthUsable(), "depth target je alokovany a pouzitelny");
            check(depth.framebuffer == nullptr, "depth target nema DIB sekci (zadne GDI)");
            check(target.framebuffer != nullptr, "barevny target ma DIB sekci");

            testQuadTiling(target, 2.0f, 2.0f, 12.0f, 12.0f, "ctverec na celych pixelech");
            testQuadTiling(target, 2.5f, 2.5f, 12.5f, 12.5f, "ctverec presne na stredech pixelu");
            testQuadTiling(target, 2.25f, 3.75f, 12.25f, 13.75f, "ctverec na zlomkovych souradnicich");
            testQuadTiling(target, 6.1f, 4.9f, 31.7f, 28.3f, "vetsi ctverec, zlomkove souradnice");

            testFanNoOverlap(target);
            testSubpixelShift(target);
            testDepthOrder(target, depth);
        }
        catch (const std::exception& e)
        {
            check(false, std::string("vyjimka pri tvorbe targetu: ") + e.what());
        }
    }

}
