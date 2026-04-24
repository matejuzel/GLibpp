#pragma once

#include <cstdint>
#include "Viewport.h"
#include "Mtx4.h"
#include "Color.h"

namespace Render {
	
    template<typename Device, typename Target>
    class DeviceContext {

    public:
        DeviceContext() = default;
        ~DeviceContext() = default;

        //Target* target = nullptr;
        Viewport viewport = { 0,0,800,600 };
        Mtx4 view = Mtx4::Identity();
        Mtx4 projection = Mtx4::Identity();
        Color clearColor = { 0,0,0,255 };
        uint32_t frameIndex = 0;


        typename Device::TargetHandle framebufferHandle = Device::TARGET_INVALID;
        typename Device::TargetHandle depthbufferHandle = Device::TARGET_INVALID;

        //typename Device::MeshHandle mesh = { 0 };
        //typename Device::MaterialHandle material = { 0 };
    };

}

