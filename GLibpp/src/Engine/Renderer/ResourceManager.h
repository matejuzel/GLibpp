#pragma once

#include "Mesh.h"
#include "StableRegistry.h"
#include "AssetRegistry.h"
#include "VertexBuffer.h"
#include "MeshInstance.h"


#include <vector>


namespace Render {

    template <typename Device>
    struct ResourceManager {

		using TargetHandle = typename Device::TargetHandle;
		using MeshInstanceHandle = typename uint32_t; // @todo

		ResourceManager(Device& device)
			: device(device)
		{

			auto& window = device.getWindow();
			auto width = window.getClientWidth();
			auto height = window.getClientHeight();

			framebufferHandle = targetCreate(RenderTargetDescriptor::FramebufferRGBA32bit(width, height));
			depthbufferHandle = targetCreate(RenderTargetDescriptor::Depthbuffer24bit(width, height));
		}

		TargetHandle targetCreate(RenderTargetDescriptor descriptor)
		{
			return device.targetCreate(descriptor);
		}

		MeshInstanceHandle meshRegister(const MeshInstance& mesh)
		{
			// @todo
			return 0;
		}


		Device& device;

		TargetHandle framebufferHandle;
		TargetHandle depthbufferHandle;

    };

}







