#pragma once

#include "DeviceContext.h"

namespace Render {

    template<typename Device, typename DerivedTarget>
    class DeviceTargetBase
    {
    public:

        RenderTargetDescriptor descriptor;

        DeviceTargetBase() = default;
        DeviceTargetBase(const RenderTargetDescriptor& descriptor) :descriptor(descriptor) {}
        ~DeviceTargetBase() = default;
    };

}