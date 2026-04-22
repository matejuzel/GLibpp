#pragma once

#include <cstdint>

namespace Render {

    enum class TextureUsage : uint8_t {
        ColorAttachment,
        DepthAttachment,
        ShaderResource,
        Storage,
    };

}