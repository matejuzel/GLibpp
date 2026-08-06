#pragma once

#include "DeviceContext.h"

namespace GLibpp::Render {

    template<typename Device>
    class DeviceTargetBase
    {
    public:
        
        RenderTargetDescriptor descriptor;

        DeviceTargetBase() = default;
        DeviceTargetBase(const RenderTargetDescriptor& descriptor) :descriptor(descriptor) {}
        ~DeviceTargetBase() = default;
    };

}