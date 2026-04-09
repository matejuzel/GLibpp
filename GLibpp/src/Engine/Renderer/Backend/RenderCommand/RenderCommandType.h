#pragma once

#include <cstdint>

enum class RenderCommandType : uint8_t {
    SetViewport,
    SetMatrixProjection,
    SetMatrixView,
    BindMesh,
    BindTarget,
    Draw,
    Clear,
};