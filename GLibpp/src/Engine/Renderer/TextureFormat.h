#pragma once 

#include <cstdint>

enum class TextureFormat : uint8_t {
    RGBA32F,
    Depth32F,
    RGBA8,
    RGBA16F,
    Depth24,
    Depth24Stencil8,
};

