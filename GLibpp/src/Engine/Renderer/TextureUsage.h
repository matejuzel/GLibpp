#pragma once

#include <cstdint>

enum class TextureUsage : uint8_t {
    ColorAttachment,
    DepthAttachment,
    ShaderResource,
    Storage,
};