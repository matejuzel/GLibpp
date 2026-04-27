#pragma once
#include <cstdint>


namespace Render {

    template <typename Device>
    struct VertexBuffer {

        enum ActiveBuffer {
            POSITION = 1 << 0,
            NORMAL = 1 << 1,
            UV = 1 << 2
        };

        uint32_t activeBuffer = POSITION | UV;

        // SoA layout
        typename Device::PositionBuffer positions;
        typename Device::VectorBuffer normals;
        typename Device::UVBuffer uvs;

    };

}
