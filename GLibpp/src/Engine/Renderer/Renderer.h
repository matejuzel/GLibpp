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

#include "RunState.h"
#include "HighResTimer.h"
#include "FixedTimestep.h"
#include "Fps.h"


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
        
        Viewport viewport;
        
        RunState running;
        ResizeRequest resizeRequest;

        ResourceManager resources;

    public:

		ResourceManager& getResourceManager() { return resources; }

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
		    
            running.start();

            Fps fps(1.0f);
            FixedTimestep step(1.0f);
            HighResTimer timer;

			uint32_t frameIndex = 0;
            while (running.isRunning())
            {

				renderFrame(camera, frameIndex++);

                {
                    // Zpracování požadavku na zmìnu velikosti
                    uint32_t w, h;
                    if (resizeRequest.consume(w, h))
                    {
                        this->resize(w, h);
                    }
                }

                {
                    double frameTime = timer.tick();
                    if (frameTime > 0.25) frameTime = 0.25;

                    if (step.consumeAll(frameTime) > 0)
                    {
                        device.getWindow().setTitle(std::format("Frame: {}, dt: {:.4f}s, FPS: {}", frameIndex, frameTime, fps.getFps()));
                    }
                    fps.tick();
                }
            }
        }

        void stop()
		{
			running.stop();
		}
  
        void resizeRequestSet(uint32_t width, uint32_t height)
        {   
            resizeRequest.set(width, height);
        }

    private:

        // Metoda resize(w, h) nesmi byt volana z jineho threadu
        void resize(uint32_t width, uint32_t height) 
        {
            if (width == 0 || height == 0) return;
            resources.framebufferHandle = device.targetResize(resources.framebufferHandle, width, height);
            resources.depthbufferHandle = device.targetResize(resources.depthbufferHandle, width, height);
            viewport.resize(width, height);
        }
    };

}