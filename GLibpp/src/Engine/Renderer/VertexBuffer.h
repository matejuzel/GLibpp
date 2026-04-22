#pragma once

namespace Render {

    template <typename Device>
    struct VertexBuffer {

        // SoA layout
        typename Device::PositionBuffer positions;
        typename Device::VectorBuffer normals;
        typename Device::UVBuffer uvs;

    };

}
