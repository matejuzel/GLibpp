#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstdint>
#include <algorithm>
#include <windows.h>
#include "window/DIB/DIBFramebuffer.h"
#include "math/Vec4.h"

class DIBRenderer {
private:
    DIBFramebuffer& fb;

public:
    DIBRenderer(DIBFramebuffer& framebuffer)
        : fb(framebuffer) {
    }

    void clear(uint32_t color) {
        uint32_t* buffer = fb.data();
        size_t count = fb.getWidth() * fb.getHeight();
        for (size_t i = 0; i < count; i++)
            buffer[i] = color;
    }

    void putPixel(int x, int y, uint32_t color) {
        if (x < 0 || y < 0 || x >= (int)fb.getWidth() || y >= (int)fb.getHeight())
            return;

        fb.data()[y * fb.getWidth() + x] = color;
    }

    void drawTriangle(const Vec4& a, const Vec4& b, const Vec4& c,
        uint32_t color, bool useAntialiasing);

    void drawCircle(int cx, int cy, int r, uint32_t color) {
        for (int y = -r; y <= r; y++)
            for (int x = -r; x <= r; x++)
                if (x * x + y * y <= r * r)
                    putPixel(cx + x, cy + y, color);
    }
};
