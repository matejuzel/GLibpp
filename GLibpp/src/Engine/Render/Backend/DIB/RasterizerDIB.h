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
        static void inline drawTriangle(
            DeviceTargetDIB& target,
            DeviceTargetDIB* depth,
            int x0, int y0, float z0,
            int x1, int y1, float z1,
            int x2, int y2, float z2,
            uint32_t color,
            bool wireframe
        ) noexcept
        {
            if (wireframe)
            {
                // jen obrys
                drawLine(target, x0, y0, x1, y1, color);
                drawLine(target, x1, y1, x2, y2, color);
                drawLine(target, x2, y2, x0, y0, color);
                return;
            }

            // Rovina hloubky - pocita se z nesetridenych vrcholu a referencni bod se
            // odlozi, takze nasledne serazeni podle Y se ji nedotkne
            const int   zRefX = x0;
            const int   zRefY = y0;
            const float zRef = z0;
            float dzdx = 0.0f;
            float dzdy = 0.0f;

            if (depth)
            {
                const float e1x = float(x1 - x0), e1y = float(y1 - y0);
                const float e2x = float(x2 - x0), e2y = float(y2 - y0);
                const float denom = e1x * e2y - e2x * e1y; // dvojnasobek plochy

                if (std::fabs(denom) < 1e-6f) return; // degenerovany trojuhelnik (nulova plocha)

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
                    float z = zRef + dzdx * float(xStart - zRefX) + dzdy * float(y - zRefY);

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


        // x, y se zaokrouhli na pixel, hloubka zustava float
        // (pozn.: chybi subpixel presnost - vrcholy se pri pohybu kamery viditelne cukaji)
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
            drawTriangle(target, depth,
                static_cast<int>(x0), static_cast<int>(y0), z0,
                static_cast<int>(x1), static_cast<int>(y1), z1,
                static_cast<int>(x2), static_cast<int>(y2), z2,
                color,
                wireframe
            );
        }

    };

}