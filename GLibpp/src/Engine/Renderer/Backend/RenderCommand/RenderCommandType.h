#pragma once

#include <cstdint>

namespace Render {

    enum class RenderCommandType : uint8_t {
        SetViewport,
        SetMatrixProjection,
        SetMatrixView,
        BindMesh,
        BindTarget,
        Draw,
        Clear,
    };

}