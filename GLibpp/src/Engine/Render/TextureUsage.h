#pragma once

#include <cstdint>

namespace GLibpp::Render {

    enum class TextureUsage : uint8_t {
        ColorAttachment,
        DepthAttachment,
        ShaderResource,
        Storage,
    };

}