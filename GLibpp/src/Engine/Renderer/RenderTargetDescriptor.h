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

    RenderTargetDescriptor() = default;

    RenderTargetDescriptor(uint32_t w, uint32_t h,
        TextureFormat fmt,
        TextureUsage use,
        uint8_t samples = 1,
        uint8_t mipLevels = 1)
        : width(w), height(h),
        format(fmt), usage(use),
        samples(samples), mipLevels(mipLevels)
    {}

    static RenderTargetDescriptor Framebuffer(uint32_t width, uint32_t height) {
        return RenderTargetDescriptor(
            width, height,
            TextureFormat::RGBA32F,
            TextureUsage::ColorAttachment,
            1,1
        );
    }
};