#pragma once

#include <cstdint>
#include <algorithm>

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    // Konstruktor z 0xAARRGGBB
    constexpr Color(uint32_t rgba)
    {
        a = (rgba >> 24) & 0xFF;
        r = (rgba >> 16) & 0xFF;
        g = (rgba >> 8) & 0xFF;
        b = (rgba >> 0) & 0xFF;
    }

    // Konstruktor z jednotlivých složek
    constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r), g(g), b(b), a(a)
    {
    }

    // Složení zpìt do 0xAARRGGBB
    constexpr uint32_t toRGBA() const noexcept {
        return (uint32_t(a) << 24) |
            (uint32_t(r) << 16) |
            (uint32_t(g) << 8) |
            uint32_t(b);
    }

    // Pøeddefinované barvy
    static constexpr Color Red() noexcept { return Color(255, 0, 0); }
    static constexpr Color Green() noexcept { return Color(0, 255, 0); }
    static constexpr Color Blue() noexcept { return Color(0, 0, 255); }
    static constexpr Color White() noexcept { return Color(255, 255, 255); }
    static constexpr Color Black() noexcept { return Color(0, 0, 0); }

    static constexpr Color Grayscale(float factor) noexcept
    {
        const float f =
            (factor < 0.0f) ? 0.0f :
            (factor > 1.0f) ? 1.0f :
            factor;

        const uint8_t v = static_cast<uint8_t>(f * 255.0f);
        return Color(v, v, v, 255);
    }


};

