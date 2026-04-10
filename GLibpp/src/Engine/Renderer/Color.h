#pragma once

#include <cstdint>

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    uint32_t toRGBA() const {
        return (uint32_t(r) << 24) |
            (uint32_t(g) << 16) |
            (uint32_t(b) << 8) |
            uint32_t(a);
    }

};
