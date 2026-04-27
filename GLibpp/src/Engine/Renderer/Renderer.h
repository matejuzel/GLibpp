#pragma once

#include "RenderTargetDescriptor.h"
#include "Viewport.h"
#include "Color.h"
#include "WindowWin32.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include "Vec4.h"
#include "Mtx4.h"

#include "Camera.h"
// Backend Common
#include "DeviceContext.h"
#include "DeviceBase.h"
#include "DeviceTargetBase.h"

// Backend DIB
#include "DeviceDIB.h"

// Backend Stencil
//#include "RenderDeviceStencil.h"


#include "ResourceManager.h"

#include "Mesh.h"

#include "StableRegistry.h"
#include "AssetRegistry.h"


namespace Render {

    template <typename Device>
    class Renderer {
    private:

        using ResourceManager = ResourceManager<Device>;

        Device device;
        ResourceManager resources;
        
        Viewport viewport;

    public:
        Renderer(WindowWin32& window)
            : device(window)
			, resources(device)
			, viewport{ 0, 0, window.getClientWidth(), window.getClientHeight() }
        {
			resize(window.getClientWidth(), window.getClientHeight());

            std::cout << "frame buffer: " << resources.framebufferHandle << std::endl;
            std::cout << "depth buffer: " << resources.depthbufferHandle << std::endl;
        }

        void renderFrame(const Camera& camera, uint32_t frameIndex)
        {   
            {
                auto ctx = device.createContext();
                
                ctx.frameIndex = frameIndex;
                ctx.clearColor = Color::Grayscale(0.4f);
                ctx.view = camera.calculateView();
                ctx.projection = Mtx4::Perspective(camera.fovRad, viewport.computeAspectRatio(), camera.nearZ, camera.farZ);
                ctx.viewport = viewport;
                ctx.framebufferHandle = resources.framebufferHandle;
                
                device.clear(ctx);
                device.drawStaticTestMesh(ctx);
            }
            
            device.present(resources.framebufferHandle);
        }

        void resize(uint32_t width, uint32_t height)
        {   
			resources.framebufferHandle = device.targetResize(resources.framebufferHandle, width, height);
            resources.depthbufferHandle = device.targetResize(resources.depthbufferHandle, width, height);

			viewport.resize(width, height);
        }
    };


}