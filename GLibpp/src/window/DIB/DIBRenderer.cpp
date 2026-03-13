
#include "window/DIB/DIBRenderer.h"
#include <algorithm>
#include <cmath>



// --- Wu helpers ---
static inline int ipart(float x) { return (int)std::floor(x); }
static inline float fpart(float x) { return x - std::floor(x); }
static inline float rfpart(float x) { return 1.0f - fpart(x); }

static inline uint32_t blendWithCoverage(uint32_t dst, uint32_t src, float coverage)
{
    coverage = (coverage < 0.0f) ? 0.0f : (coverage > 1.0f) ? 1.0f : coverage;

    uint8_t sr = (src >> 16) & 0xFF;
    uint8_t sg = (src >> 8) & 0xFF;
    uint8_t sb = (src) & 0xFF;

    uint8_t dr = (dst >> 16) & 0xFF;
    uint8_t dg = (dst >> 8) & 0xFF;
    uint8_t db = (dst) & 0xFF;

    uint8_t r = (uint8_t)(sr * coverage + dr * (1.0f - coverage));
    uint8_t g = (uint8_t)(sg * coverage + dg * (1.0f - coverage));
    uint8_t b = (uint8_t)(sb * coverage + db * (1.0f - coverage));

    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

// --- Wu line ---
static void drawLineWithAntialiasing(uint32_t* framebuffer, int fbWidth, int fbHeight,
    int x0, int y0, int x1, int y1, uint32_t color)
{
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

    if (steep) { std::swap(x0, y0); std::swap(x1, y1); }
    if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); }

    float dx = float(x1 - x0);
    float dy = float(y1 - y0);
    float gradient = dx == 0.0f ? 1.0f : dy / dx;

    auto plot = [&](int x, int y, float c) {
        if (c <= 0.0f) return;
        if (steep) std::swap(x, y);

        if (x >= 0 && x < fbWidth && y >= 0 && y < fbHeight) {
            size_t idx = size_t(y) * fbWidth + x;
            framebuffer[idx] = blendWithCoverage(framebuffer[idx], color, c);
        }
        };

    float xend = std::round(float(x0));
    float yend = y0 + gradient * (xend - x0);
    float xgap = rfpart(float(x0) + 0.5f);
    int xpxl1 = int(xend);
    int ypxl1 = ipart(yend);

    plot(xpxl1, ypxl1, rfpart(yend) * xgap);
    plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);

    float intery = yend + gradient;

    xend = std::round(float(x1));
    yend = y1 + gradient * (xend - x1);
    xgap = fpart(float(x1) + 0.5f);
    int xpxl2 = int(xend);
    int ypxl2 = ipart(yend);

    plot(xpxl2, ypxl2, rfpart(yend) * xgap);
    plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap);

    for (int x = xpxl1 + 1; x <= xpxl2 - 1; x++) {
        plot(x, ipart(intery), rfpart(intery));
        plot(x, ipart(intery) + 1, fpart(intery));
        intery += gradient;
    }
}

// --- Bresenham ---
static void drawLine(uint32_t* framebuffer, int fbWidth, int fbHeight,
    int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        if (x0 >= 0 && y0 >= 0 && x0 < fbWidth && y0 < fbHeight)
            framebuffer[y0 * fbWidth + x0] = color;

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// --- Triangle ---
void DIBRenderer::drawTriangle(const Vec4& a, const Vec4& b, const Vec4& c,
    uint32_t color, bool useAntialiasing)
{
    uint32_t* buffer = fb.data();
    int fbWidth = int(fb.getWidth());
    int fbHeight = int(fb.getHeight());

    int ax = int(std::lround(a.x));
    int ay = int(std::lround(a.y));
    int bx = int(std::lround(b.x));
    int by = int(std::lround(b.y));
    int cx = int(std::lround(c.x));
    int cy = int(std::lround(c.y));

    if (useAntialiasing) {
        drawLineWithAntialiasing(buffer, fbWidth, fbHeight, ax, ay, bx, by, color);
        drawLineWithAntialiasing(buffer, fbWidth, fbHeight, bx, by, cx, cy, color);
        drawLineWithAntialiasing(buffer, fbWidth, fbHeight, cx, cy, ax, ay, color);
    }
    else {
        drawLine(buffer, fbWidth, fbHeight, ax, ay, bx, by, color);
        drawLine(buffer, fbWidth, fbHeight, bx, by, cx, cy, color);
        drawLine(buffer, fbWidth, fbHeight, cx, cy, ax, ay, color);
    }
}
