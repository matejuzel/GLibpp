#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <iterator>
#include "DeviceTargetBase.h"
#include "DeviceTargetDIB.h"
#include "FragmentShaderId.h"
#include "FragmentShader.h"
#include "ShaderSolid.h"
#include "ShaderLambert.h"
#include "ShaderUvDebug.h"
#include "ShaderTextured.h"
#include "ShaderTexturedBilinear.h"
#include "ShaderTexturedAniso.h"


namespace GLibpp::Render {

    class RasterizerDIB {

    public:

        static void inline drawLine(DeviceTargetDIB& target, int x0, int y0, int x1, int y1, uint32_t color) noexcept
        {
            int dx = abs(x1 - x0);
            int sx = x0 < x1 ? 1 : -1;
            int dy = -abs(y1 - y0);
            int sy = y0 < y1 ? 1 : -1;
            int err = dx + dy;

            while (true)
            {
                target.putPixel(x0, y0, color);

                if (x0 == x1 && y0 == y1)
                    break;

                int e2 = 2 * err;

                if (e2 >= dy) {
                    err += dy;
                    x0 += sx;
                }

                if (e2 <= dx) {
                    err += dx;
                    y0 += sy;
                }
            }
        }

        // ---- Rasterizace trojuhelniku: hranove funkce nad subpixelovou mrizkou ----
        //
        // Pokryti rozhoduje test STREDU pixelu proti trem hranovym funkcim (pixel
        // s indexem x pokryva interval [x, x+1), takze jeho stred je x+0.5).
        // Vrcholy se nezaokrouhluji na cele pixely - snapuji se na mrizku
        // 1/kSubScale pixelu, takze hrany pri pohybu kamery necukaji.
        //
        // Proc fixed-point a ne float: hranova funkce musi byt EXAKTNI. Pro hranu
        // sdilenou dvema trojuhelniky plati algebraicky E_AB(p) == -E_BA(p), a v
        // celych cislech to plati na bit. Ve floatu by zaokrouhleni mohlo pixel
        // lezici presne na hrane dat obema (dvojity zapis) nebo zadnemu (svy).
        // Diky exaktnosti staci top-left fill rule a hranicni pixel patri PRESNE
        // JEDNOMU trojuhelniku - sousedni plosky se uz o nej nehadaji.
        //
        // Fragment shader: barvu pixelu urcuje Shader::shade(), ktery se do
        // vnitrni smycky INLINUJE (parametr sablony, zadny per-pixel dispatch);
        // per-triangle priprava (Shader::setup) probehne jednou. Vyber shaderu
        // za behu jde pres fragmentFunction(id) - LUT instanciaci teto sablony.
        //
        // Hloubka: NDC z je po perspektivnim deleni afinni funkci obrazovkovych
        // x, y, takze staci rovina z = zRef + dzdx*dx + dzdy*dy (1/w interpolace
        // je potreba az pro atributy typu UV, ne pro hloubku). Rovina se pocita
        // z tychze snapnutych souradnic jako pokryti, vzorkuje se ve stredu
        // pixelu a pocita se VZDY - interpolovane z dostava i shader
        // (PixelInput.z), ne jen depth test. Depth test je "mensi vyhrava" a
        // shade() se vola az po nem (early-z); bez depth targetu (nullptr) se
        // kresli v poradi commandu jako dosud.
        // Pozn.: u extremne tenkych trojuhelniku je jmenovatel roviny maly a
        // gradient hloubky velky - presna matematika drzi z uvnitr [min z, max z]
        // vrcholu, float zaokrouhleni se tam ale znatelne zesiluje.
        //
        // Obrys (wireframe) bere barvu ze shade() v centroidu (zachovava napr.
        // stinovany dratovy model vlny); cary hloubku ignoruji - debug overlay.
        template <FragmentShader Shader>
        static void rasterizeTriangleT(
            DeviceTargetDIB& target,
            DeviceTargetDIB* depth,
            const TriangleInput& tri,
            const ShaderUniforms& uniforms
        ) noexcept
        {
            // per-triangle "varyings" - u plochych shaderu tady probehne vsechno
            const auto triCtx = Shader::setup(tri, uniforms);

            if (tri.wireframe)
            {
                // jen obrys - cary adresuji pixel obsahujici vrchol;
                // atributy centroidu = prumery (pro debug obrys staci)
                const PixelInput centroid{
                    pixelOf((tri.x0 + tri.x1 + tri.x2) / 3.0f),
                    pixelOf((tri.y0 + tri.y1 + tri.y2) / 3.0f),
                    (tri.z0 + tri.z1 + tri.z2) / 3.0f,
                    (tri.u0 * tri.invW0 + tri.u1 * tri.invW1 + tri.u2 * tri.invW2) / 3.0f,
                    (tri.v0 * tri.invW0 + tri.v1 * tri.invW1 + tri.v2 * tri.invW2) / 3.0f,
                    (tri.invW0 + tri.invW1 + tri.invW2) / 3.0f
                };
                const uint32_t color = Shader::shade(triCtx, centroid, uniforms);

                const int lx0 = pixelOf(tri.x0), ly0 = pixelOf(tri.y0);
                const int lx1 = pixelOf(tri.x1), ly1 = pixelOf(tri.y1);
                const int lx2 = pixelOf(tri.x2), ly2 = pixelOf(tri.y2);

                drawLine(target, lx0, ly0, lx1, ly1, color);
                drawLine(target, lx1, ly1, lx2, ly2, color);
                drawLine(target, lx2, ly2, lx0, ly0, color);
                return;
            }

            // --- 1) snap vrcholu na subpixelovou mrizku ---
            int64_t vx[3] = { toFixed(tri.x0), toFixed(tri.x1), toFixed(tri.x2) };
            int64_t vy[3] = { toFixed(tri.y0), toFixed(tri.y1), toFixed(tri.y2) };
            float   vz[3] = { tri.z0, tri.z1, tri.z2 };

            // atributy pro perspektivne korektni interpolaci: u/w, v/w a 1/w
            // jsou (na rozdil od u, v samotnych) afinni v obrazovkovem prostoru,
            // takze se interpoluji rovinami jako hloubka; shader deli az per pixel
            float uw[3] = { tri.u0 * tri.invW0, tri.u1 * tri.invW1, tri.u2 * tri.invW2 };
            float vw[3] = { tri.v0 * tri.invW0, tri.v1 * tri.invW1, tri.v2 * tri.invW2 };
            float iw[3] = { tri.invW0, tri.invW1, tri.invW2 };

            // --- 2) vinuti: chceme "uvnitr" == vsechny hranove funkce >= 0 ---
            const int64_t area2 = (vx[1] - vx[0]) * (vy[2] - vy[0])
                                - (vy[1] - vy[0]) * (vx[2] - vx[0]);

            if (area2 == 0) return; // nulova plocha - neni co rasterizovat

            if (area2 < 0)
            {
                // obracene vinuti - prohodime dva vrcholy vcetne vsech atributu
                std::swap(vx[1], vx[2]);
                std::swap(vy[1], vy[2]);
                std::swap(vz[1], vz[2]);
                std::swap(uw[1], uw[2]);
                std::swap(vw[1], vw[2]);
                std::swap(iw[1], iw[2]);
            }

            // --- 3) hranove funkce ve tvaru E_i(p) = A_i*p.x + B_i*p.y + C_i ---
            int64_t A[3], B[3], C[3];
            for (int i = 0; i < 3; ++i)
            {
                const int j = (i + 1) % 3;
                const int64_t dx = vx[j] - vx[i];
                const int64_t dy = vy[j] - vy[i];

                A[i] = -dy;
                B[i] = dx;
                C[i] = dy * vx[i] - dx * vy[i];

                // top-left fill rule: pixel presne na hrane (E == 0) se pocita jen
                // u horni a leve hrany. Sousedni trojuhelnik vidi tutez hranu s
                // obracenym smerem, takze podminku splni presne jeden z nich.
                const bool topLeft = (dy < 0) || (dy == 0 && dx > 0);
                if (!topLeft) C[i] -= 1; // z ">= 0" se stane "> 0"
            }

            // --- 4) rovina hloubky z tychze snapnutych souradnic ---
            const float px[3] = { fromFixed(vx[0]), fromFixed(vx[1]), fromFixed(vx[2]) };
            const float py[3] = { fromFixed(vy[0]), fromFixed(vy[1]), fromFixed(vy[2]) };

            const float e1x = px[1] - px[0], e1y = py[1] - py[0];
            const float e2x = px[2] - px[0], e2y = py[2] - py[0];
            const float denom = e1x * e2y - e2x * e1y;

            if (std::fabs(denom) < 1e-6f) return; // degenerovany trojuhelnik

            const float inv = 1.0f / denom;

            // gradient roviny atributu a(x, y) = a[0] + dadx*dx + dady*dy
            auto planeGrad = [&](const float a[3], float& dadx, float& dady) noexcept {
                dadx = ((a[1] - a[0]) * e2y - (a[2] - a[0]) * e1y) * inv;
                dady = ((a[2] - a[0]) * e1x - (a[1] - a[0]) * e2x) * inv;
            };

            float dzdx, dzdy;   planeGrad(vz, dzdx, dzdy);
            float duwdx, duwdy; planeGrad(uw, duwdx, duwdy);
            float dvwdx, dvwdy; planeGrad(vw, dvwdx, dvwdy);
            float diwdx, diwdy; planeGrad(iw, diwdx, diwdy);

            // --- 5) obalka v pixelech (kandidati, jejichz stred muze byt uvnitr) ---
            const float minX = std::min(px[0], std::min(px[1], px[2]));
            const float maxX = std::max(px[0], std::max(px[1], px[2]));
            const float minY = std::min(py[0], std::min(py[1], py[2]));
            const float maxY = std::max(py[0], std::max(py[1], py[2]));

            const int bxMin = std::max(0, static_cast<int>(std::floor(minX - 0.5f)));
            const int bxMax = std::min(static_cast<int>(target.descriptor.width) - 1,
                                       static_cast<int>(std::ceil(maxX - 0.5f)));
            const int byMin = std::max(0, static_cast<int>(std::floor(minY - 0.5f)));
            const int byMax = std::min(static_cast<int>(target.descriptor.height) - 1,
                                       static_cast<int>(std::ceil(maxY - 0.5f)));

            if (bxMin > bxMax || byMin > byMax) return;

            // --- 6) prochazeni: hranove funkce se jen pricitaji ---
            const int64_t stepX[3] = { A[0] * kSubScale, A[1] * kSubScale, A[2] * kSubScale };
            const int64_t stepY[3] = { B[0] * kSubScale, B[1] * kSubScale, B[2] * kSubScale };

            // stred prvniho pixelu obalky
            const int64_t originX = int64_t(bxMin) * kSubScale + kSubScale / 2;
            const int64_t originY = int64_t(byMin) * kSubScale + kSubScale / 2;

            int64_t rowE[3];
            for (int i = 0; i < 3; ++i)
                rowE[i] = A[i] * originX + B[i] * originY + C[i];

            const size_t stride = target.descriptor.width;
            float* depthBase = depth ? depth->depthbuffer.data() : nullptr;

            for (int y = byMin; y <= byMax; ++y)
            {
                int64_t e0 = rowE[0], e1 = rowE[1], e2 = rowE[2];

                uint32_t* row = target.framebuffer + size_t(y) * stride;
                float* depthRow = depthBase ? depthBase + size_t(y) * stride : nullptr;

                const float startDX = (float(bxMin) + 0.5f) - px[0];
                const float startDY = (float(y) + 0.5f) - py[0];

                float z   = vz[0] + dzdx  * startDX + dzdy  * startDY;
                float uwv = uw[0] + duwdx * startDX + duwdy * startDY;
                float vwv = vw[0] + dvwdx * startDX + dvwdy * startDY;
                float iwv = iw[0] + diwdx * startDX + diwdy * startDY;

                bool wasInside = false;

                for (int x = bxMin; x <= bxMax; ++x)
                {
                    // vsechny tri hranove funkce nezaporne (staci znamenkovy bit v OR)
                    if ((e0 | e1 | e2) >= 0)
                    {
                        wasInside = true;

                        if (!depthRow)
                        {
                            row[x] = Shader::shade(triCtx, PixelInput{ x, y, z, uwv, vwv, iwv }, uniforms);
                        }
                        else if (z < depthRow[x])
                        {
                            // early-z: shade() se vola az po vyhranem depth testu
                            depthRow[x] = z;
                            row[x] = Shader::shade(triCtx, PixelInput{ x, y, z, uwv, vwv, iwv }, uniforms);
                        }
                    }
                    else if (wasInside)
                    {
                        break; // trojuhelnik je konvexni - uz jsme z nej vyjeli
                    }

                    e0 += stepX[0]; e1 += stepX[1]; e2 += stepX[2];
                    z += dzdx; uwv += duwdx; vwv += dvwdx; iwv += diwdx;
                }

                rowE[0] += stepY[0]; rowE[1] += stepY[1]; rowE[2] += stepY[2];
            }
        }

        // Kompatibilni vstup bez vyberu shaderu: plna barva (ShaderSolid).
        // Drzi presne hodnoty barev - stoji na tom testy rasterizace.
        // Roundtrip uint32 -> Color -> toRGBA je bezztratovy (oboji 0xAARRGGBB).
        static void inline drawTriangle(
            DeviceTargetDIB& target,
            DeviceTargetDIB* depth,
            float x0, float y0, float z0,
            float x1, float y1, float z1,
            float x2, float y2, float z2,
            uint32_t color,
            bool wireframe
        ) noexcept
        {
            const TriangleInput tri{
                x0, y0, z0,
                x1, y1, z1,
                x2, y2, z2,
                0.0f, 0.0f, 1.0f,   // UV nulove, invW = 1 (afinni pripad)
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 0.0f,   // normala - ShaderSolid ji nepouziva
                Color(color),
                wireframe
            };
            // uniformy z rozmeru targetu (descriptor garantuje >= 1), bez textury
            const ShaderUniforms uniforms{
                1.0f / float(target.descriptor.width),
                1.0f / float(target.descriptor.height),
                nullptr, 0, 0
            };
            rasterizeTriangleT<ShaderSolid>(target, depth, tri, uniforms);
        }

        // typ radku dispatch tabulky: rasterizace trojuhelniku konkretnim shaderem
        using RasterizeFn = void (*)(DeviceTargetDIB&, DeviceTargetDIB*, const TriangleInput&, const ShaderUniforms&) noexcept;

        // O(1) vyber shaderu: tabulka ukazatelu na instanciace rasterizeTriangleT,
        // index = FragmentShaderId, poradi radku = poradi enumu (hlida static_assert
        // - stejny vzor jako dispatch tabulka DrawCommandu v Rendereru).
        // Fetch se dela jednou na draw (viz rasterizeMesh) -> jeden neprimy call
        // na trojuhelnik, per-pixel dispatch neexistuje (shade je inlinovany).
        static RasterizeFn fragmentFunction(FragmentShaderId id) noexcept
        {
            static constexpr RasterizeFn kFragmentDispatch[] = {
                &rasterizeTriangleT<ShaderSolid>,
                &rasterizeTriangleT<ShaderLambert>,
                &rasterizeTriangleT<ShaderUvDebug>,
                &rasterizeTriangleT<ShaderTextured>,
                &rasterizeTriangleT<ShaderTexturedBilinear>,
                &rasterizeTriangleT<ShaderTexturedAniso>,
            };
            static_assert(std::size(kFragmentDispatch) == static_cast<size_t>(FragmentShaderId::Count),
                "dispatch tabulka musi pokryvat vsechny druhy shaderu");

            return kFragmentDispatch[static_cast<size_t>(id)];
        }

    private:

        // 1 pixel = kSubScale jednotek mrizky; 8 bitu (1/256 px) je stejna
        // presnost, jakou pro souradnice pouzivaji GPU pipeline
        static constexpr int kSubBits = 8;
        static constexpr int64_t kSubScale = int64_t(1) << kSubBits;

        static int64_t inline toFixed(float v) noexcept
        {
            return static_cast<int64_t>(std::llround(v * float(kSubScale)));
        }

        static float inline fromFixed(int64_t v) noexcept
        {
            return static_cast<float>(v) / static_cast<float>(kSubScale);
        }

        // index pixelu, ktery dany spojity bod obsahuje
        static int inline pixelOf(float v) noexcept
        {
            return static_cast<int>(std::floor(v));
        }

    };

}
