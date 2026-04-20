#pragma once

#include "SlotArray.h"

template <typename Device>
struct RenderResourceManager {
public:
    RenderResourceManager() = default;

    // types
    using Target_h = Device::TargetHandle;
    using Mesh_h = uint32_t;
    using MeshInstance_h = uint32_t;
    using Texture_h = uint32_t;
    

    // properities
    SlotArray<typename Device::Target> targets;

    // methods


};