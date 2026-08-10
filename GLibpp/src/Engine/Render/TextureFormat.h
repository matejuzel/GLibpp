#pragma once 

#include <cstdint>

namespace GLibpp::Render {

    enum class TextureFormat : uint8_t {
        RGBA32F,
        Depth32F,
        RGBA8,
        RGBA16F,
        Depth24,
        Depth24Stencil8,
    };

    // Rozhoduje o residency targetu: hloubka je u DIB backendu souvisle pole floatu
    // bez GDI, barva je DIB sekce s HBITMAP/HDC (viz DeviceTargetDIB).
    inline constexpr bool isDepthFormat(TextureFormat format) noexcept {
        return format == TextureFormat::Depth32F
            || format == TextureFormat::Depth24
            || format == TextureFormat::Depth24Stencil8;
    }

}

