#pragma once

#include <cstdint>
#include <algorithm>
#include "RenderTargetBase.h"
#include "RenderTargetDIB.h"


class RasterizatorDIB {

public:

    static void inline drawLine(RenderTargetDIB& target, int x0, int y0, int x1, int y1, uint32_t color) noexcept
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

    static void inline drawTriangle(RenderTargetDIB& target, int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) noexcept
    {
        // Seøadíme vrcholy podle Y (od nejnižšího)
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

                uint32_t* row = target.framebuffer + y * target.descriptor.width;
                for (int x = xStart; x <= xEnd; x++)
                    row[x] = color;
            };

        auto edgeInterp = [&](int y, int x0, int y0, int x1, int y1)
            {
                if (y1 == y0)
                    return x0;

                return x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            };

        // Horní èást (od y0 do y1)
        for (int y = y0; y <= y1; y++)
        {
            int xa = edgeInterp(y, x0, y0, x2, y2);
            int xb = edgeInterp(y, x0, y0, x1, y1);
            drawSpan(y, xa, xb);
        }

        // Dolní èást (od y1 do y2)
        for (int y = y1; y <= y2; y++)
        {
            int xa = edgeInterp(y, x0, y0, x2, y2);
            int xb = edgeInterp(y, x1, y1, x2, y2);
            drawSpan(y, xa, xb);
        }
    }

    static void inline drawQuad(RenderTargetDIB& target, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) noexcept
    {
        drawTriangle(target, x0, y0, x1, y1, x2, y2, color);
        drawTriangle(target, x0, y0, x2, y2, x3, y3, color);
    }

    static void inline drawQuad(RenderTargetDIB& target, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color) noexcept
    {
        drawQuad(target,
            static_cast<int>(x0),
            static_cast<int>(y0),
            static_cast<int>(x1),
            static_cast<int>(y1),
            static_cast<int>(x2),
            static_cast<int>(y2),
            static_cast<int>(x3),
            static_cast<int>(y3),
            color
        );
    }


};