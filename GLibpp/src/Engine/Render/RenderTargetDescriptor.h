#pragma once

#include <cstdint>
#include "TextureFormat.h"
#include "TextureUsage.h"

namespace GLibpp::Render {

    struct RenderTargetDescriptor {
        // NSDMI jsou nutne: default-konstruovany descriptor drive nesl neurcitou
        // velikost i format a target z nej postaveny mel garbage rozmery
        uint32_t width = 0;
        uint32_t height = 0;
        TextureFormat format = TextureFormat::RGBA8;
        TextureUsage usage = TextureUsage::ColorAttachment;

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
        {
        }

        // 32 bpp barva = 8 bitu na kanal (tak vypada DIB sekce);
        // RGBA32F by byl 128 bpp float per pixel - drive tu bylo omylem prave to
        static RenderTargetDescriptor FramebufferRGBA32bit(uint32_t width, uint32_t height) {
            return RenderTargetDescriptor(
                width, height,
                TextureFormat::RGBA8,
                TextureUsage::ColorAttachment,
                1, 1
            );
        }
        // textura pro sampling ve fragment shaderu - u DIB backendu texely
        // v pameti bez GDI (viz DeviceTargetDIB::texelStorage)
        static RenderTargetDescriptor Texture(uint32_t width, uint32_t height) {
            return RenderTargetDescriptor(
                width, height,
                TextureFormat::RGBA8,
                TextureUsage::ShaderResource,
                1, 1
            );
        }

        // hloubka jako float - DIB backend testuje NDC z (viz kDepthFar v DeviceDIB);
        // usage byl drive copy-paste ColorAttachment
        static RenderTargetDescriptor Depthbuffer32F(uint32_t width, uint32_t height) {
            return RenderTargetDescriptor(
                width, height,
                TextureFormat::Depth32F,
                TextureUsage::DepthAttachment,
                1, 1
            );
        }
    };

}