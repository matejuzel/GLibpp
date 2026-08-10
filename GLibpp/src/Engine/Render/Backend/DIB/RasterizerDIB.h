#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include "DeviceTargetBase.h"
#include "DeviceTargetDIB.h"


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

        // Depth test se dela, jen kdyz je predan depth target (nullptr = kresli se
        // v poradi jako dosud). Hloubka na vstupu je NDC z v [-1, 1] (-1 = near),
        // test je "mensi vyhrava" a vyhra znamena zapis do framebufferu i do hloubky.
        //
        // Interpolace je presna a levna: po perspektivnim deleni je NDC z afinni
        // funkci obrazovkovych x, y, takze staci rovina z = zRef + dzdx*dx + dzdy*dy
        // a v ramci radku se pricita dzdx. (1/w interpolace je potreba az pro
        // atributy typu UV nebo barvy, ne pro hloubku.)
        //
        // Obrys (wireframe) i drawLine hloubku ignoruji - cary jsou debug overlay.
        //
        // Pro sousedni trojuhelniky jsou kriticke dve veci:
        //  1) rovina se pocita z NEZAOKROUHLENYCH souradnic vrcholu. Fitovana na cela
        //     cisla dava dvema trojuhelnikum se spolecnou hranou meritelne odlisne
        //     roviny - na sdilene hrane se pak hadaji o hloubku a vyhrava ten, ktery
        //     se zaokrouhlenim posunul blize (viditelne artefakty na navazujicich
        //     hranach). Z presnych souradnic obe roviny na sdilene hrane splynou.
        //  2) hloubka se vzorkuje ve STREDU pixelu (x+0.5, y+0.5) - pixel s indexem x
        //     pokryva interval [x, x+1). Vzorkovani v rohu zanasi systematickou chybu
        //     0,5*(dzdx + dzdy), ktera je pro kazdy trojuhelnik jina, takze i dve
        //     spravne roviny se na sdilene hrane rozejdou.
        //
        // Pokryti se porad urcuje ze zaokrouhlenych vrcholu (chybi subpixel presnost
        // a exclusive fill rule - viz ANALYZA-CODEBASE.md), takze pixel na sdilene
        // hrane kresli oba trojuhelniky. Se striktnim < ale vyhrava ten prvni a
        // vysledek je deterministicky, ne per-pixel nahodny.
        static void inline drawTriangle(
            DeviceTargetDIB& target,
            DeviceTargetDIB* depth,
            float fx0, float fy0, float z0,
            float fx1, float fy1, float z1,
            float fx2, float fy2, float z2,
            uint32_t color,
            bool wireframe
        ) noexcept
        {
            int x0 = static_cast<int>(fx0), y0 = static_cast<int>(fy0);
            int x1 = static_cast<int>(fx1), y1 = static_cast<int>(fy1);
            int x2 = static_cast<int>(fx2), y2 = static_cast<int>(fy2);

            if (wireframe)
            {
                // jen obrys
                drawLine(target, x0, y0, x1, y1, color);
                drawLine(target, x1, y1, x2, y2, color);
                drawLine(target, x2, y2, x0, y0, color);
                return;
            }

            // Rovina hloubky - z presnych souradnic nesetridenych vrcholu; referencni
            // bod se odlozi, takze nasledne serazeni podle Y se ji nedotkne
            const float zRefX = fx0;
            const float zRefY = fy0;
            const float zRef = z0;
            float dzdx = 0.0f;
            float dzdy = 0.0f;

            if (depth)
            {
                const float e1x = fx1 - fx0, e1y = fy1 - fy0;
                const float e2x = fx2 - fx0, e2y = fy2 - fy0;
                const float denom = e1x * e2y - e2x * e1y; // dvojnasobek plochy

                if (std::fabs(denom) < 1e-6f) return; // nulova plocha - neni co vzorkovat

                const float inv = 1.0f / denom;
                dzdx = ((z1 - z0) * e2y - (z2 - z0) * e1y) * inv;
                dzdy = ((z2 - z0) * e1x - (z1 - z0) * e2x) * inv;
            }

            // Seřadíme vrcholy podle Y (od nejnižšího)
            if (y1 < y0) { std::swap(y0, y1); std::swap(x0, x1); }
            if (y2 < y0) { std::swap(y0, y2); std::swap(x0, x2); }
            if (y2 < y1) { std::swap(y1, y2); std::swap(x1, x2); }

            auto drawSpan = [&](int y, int xStart, int xEnd)
                {
                    if (y < 0 || y >= (int)target.descriptor.height)
                        return;

                    if (xStart > xEnd)
                        std::swap(xStart, xEnd);

                    xStart = std::max(0, xStart);
                    xEnd = std::min((int)target.descriptor.width - 1, xEnd);
                    if (xStart > xEnd)
                        return;

                    const size_t stride = target.descriptor.width;
                    uint32_t* row = target.framebuffer + size_t(y) * stride;

                    if (!depth)
                    {
                        for (int x = xStart; x <= xEnd; x++)
                            row[x] = color;
                        return;
                    }

                    float* depthRow = depth->depthbuffer.data() + size_t(y) * stride;

                    // vzorkuje se stred pixelu, ne jeho levy horni roh
                    float z = zRef
                        + dzdx * ((float(xStart) + 0.5f) - zRefX)
                        + dzdy * ((float(y) + 0.5f) - zRefY);

                    for (int x = xStart; x <= xEnd; x++, z += dzdx)
                    {
                        if (z < depthRow[x])
                        {
                            depthRow[x] = z;
                            row[x] = color;
                        }
                    }
                };

            auto edgeInterp = [&](int y, int x0, int y0, int x1, int y1)
                {
                    if (y1 == y0)
                        return x0;

                    return x0 + (x1 - x0) * (y - y0) / (y1 - y0);
                };

            // Horní část (od y0 do y1)
            for (int y = y0; y <= y1; y++)
            {
                int xa = edgeInterp(y, x0, y0, x2, y2);
                int xb = edgeInterp(y, x0, y0, x1, y1);
                drawSpan(y, xa, xb);
            }

            // Dolní část (od y1 do y2)
            for (int y = y1; y <= y2; y++)
            {
                int xa = edgeInterp(y, x0, y0, x2, y2);
                int xb = edgeInterp(y, x1, y1, x2, y2);
                drawSpan(y, xa, xb);
            }
        }


    };

}