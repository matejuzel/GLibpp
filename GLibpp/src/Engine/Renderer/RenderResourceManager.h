#pragma once

#include "Mesh.h"
#include "SlotArray.h"
#include "AssetRegistry.h"
#include "VertexBuffer.h"


#include <vector>


namespace Render {

    template <typename Device>
    struct RenderResourceManager {
    public:
        RenderResourceManager() = default;

        using MeshRegistry = AssetRegistry<Mesh>;
        using MeshHandle = typename MeshRegistry::Handle;

        // types
        using TargetHandle = typename Device::TargetHandle;
        using MeshInstanceHandle = uint32_t;
        using TextureHandle = uint32_t;


        // properities
        SlotArray<typename Device::Target> targets;

        MeshRegistry meshRegistry;

        VertexBuffer<Device> vertexBuffer;

        // methods


    };

}







