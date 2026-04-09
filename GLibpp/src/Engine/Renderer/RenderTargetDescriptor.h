#pragma once

#include <cstdint>
#include "TextureFormat.h"
#include "TextureUsage.h"

struct RenderTargetDescriptor {
    uint32_t width;
    uint32_t height;
    TextureFormat format;
    TextureUsage usage;

    uint8_t samples = 1; // MSAA samples, default 1 (no MSAA)
    uint8_t mipLevels = 1; // number of mip levels, default 1 (no mipmaps)

    static RenderTargetDescriptor Framebuffer(uint32_t width, uint32_t height) {
        return {
            width, height,
            TextureFormat::RGBA32F,
            TextureUsage::ColorAttachment,
            1,1
        };
    }
};