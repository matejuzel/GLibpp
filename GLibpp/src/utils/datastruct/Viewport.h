#pragma once

#include <cstdint>

class Viewport {
public:
    Viewport(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height) : offsetX(offsetX), offsetY(offsetY), width(width), height(height) {}

    uint32_t offsetX;
    uint32_t offsetY;
    uint32_t width;
    uint32_t height;
};
