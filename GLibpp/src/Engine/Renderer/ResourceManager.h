#pragma once

#include "Mesh.h"
#include "StableRegistry.h"
#include "AssetRegistry.h"
#include "VertexBuffer.h"


#include <vector>


namespace Render {

    template <typename Device>
    struct ResourceManager {

		ResourceManager( Device& device )
			: device(device)
		{

			auto& window = device.getWindow();
			auto width = window.getClientWidth();
			auto height = window.getClientHeight();

			framebufferHandle = device.targetCreate(RenderTargetDescriptor::FramebufferRGBA32bit(width, height));
			depthbufferHandle = device.targetCreate(RenderTargetDescriptor::Depthbuffer24bit(width, height));
		}


		Device& device;

		typename Device::TargetHandle framebufferHandle;
		typename Device::TargetHandle depthbufferHandle;



    };

}







