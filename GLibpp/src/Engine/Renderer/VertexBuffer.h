#pragma once

#include "RenderDeviceBase.h"

template <typename Device>
struct VertexBuffer {

    // SoA layout
    typename Device::GpuBuffer3D positions;
    typename Device::GpuBuffer3D normals;
    typename Device::GpuBuffer2D uvs;
};
