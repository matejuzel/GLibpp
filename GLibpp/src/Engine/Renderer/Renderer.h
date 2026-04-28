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


    struct ResizeRequest {
        std::atomic<bool> active = { false };
        uint32_t width = 0;
        uint32_t height = 0;

        void set(uint32_t w, uint32_t h) {
            width = w;
            height = h;
            active.store(true, std::memory_order_release);
        }

        bool consume(uint32_t& outW, uint32_t& outH) {
            if (active.load(std::memory_order_acquire)) {
                outW = width;
                outH = height;
                active.store(false, std::memory_order_relaxed);
                return true;
            }
            return false;
        }
    };


    template <typename Device>
    class Renderer {
    private:

        using ResourceManager = ResourceManager<Device>;

        Device device;
        ResourceManager resources;
        
        Viewport viewport;

		std::atomic<bool> running;
        ResizeRequest resizeRequest;

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

        void runLoop(const Camera& camera)
        {
			start();
			uint32_t frameIndex = 0;
            while (isRunning())
            {

                {
                    // Zpracování požadavku na zmìnu velikosti
                    // takhle je to thread safe. Metoda resize() nesmi byt volana z jineho threadu
                    uint32_t w, h;
                    if (resizeRequest.consume(w, h))
                    {
                        this->resize(w, h);
                    }
                }

				renderFrame(camera, frameIndex++);
            }
        }

        bool isRunning() const noexcept
        {
            return running.load(std::memory_order_relaxed);
		}

    private:
        void start()
        {
			running.store(true, std::memory_order_relaxed);
        }


    public:
        void stop()
		{
			running.store(false, std::memory_order_relaxed);
		}

  
        void resizeRequestSet(uint32_t width, uint32_t height)
        {   
            resizeRequest.set(width, height);
        }

    private:

        void resize(uint32_t width, uint32_t height) 
        {
            resources.framebufferHandle = device.targetResize(resources.framebufferHandle, width, height);
            resources.depthbufferHandle = device.targetResize(resources.depthbufferHandle, width, height);
            viewport.resize(width, height);
        }

    };


}