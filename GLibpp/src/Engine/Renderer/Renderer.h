#pragma once

#include "RenderTargetDescriptor.h"
#include "Viewport.h"
#include "Color.h"
#include "WindowWin32.h"

#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>

#include <windows.h>

#include <memory>
#include "Vec4.h"
#include "Mtx4.h"

#include "Camera.h"
#include "DoubleBuffer.h"
// Backend Common
#include "DeviceContext.h"
#include "DeviceBase.h"
#include "DeviceTargetBase.h"

// Backend DIB
#include "DeviceDIB.h"

// Backend Stencil
//#include "RenderDeviceStencil.h"


#include "ResourceManager.h"

#include "Scene.h"

#include "Mesh.h"

#include "StableRegistry.h"
#include "AssetRegistry.h"

#include "RunState.h"
#include "TimeManager.h"

#include "ZeroAllocStateHistory.h"

#include <utility>


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
        using SceneInterpolationPair = ZeroAllocStateHistory<Scene>;

        Device device;
        Viewport viewport;
        ResizeRequest resizeRequest;
        ResourceManager resources;

        float logicHz;

        SceneBuffer& sceneBuffered;
        RunState running;

    public:

        Renderer(WindowWin32& window, SceneBuffer& sceneBuffered, float logicHz)
            : device(window)
			, resources(device)
			, viewport{ 0, 0, window.getClientWidth(), window.getClientHeight() }
            , sceneBuffered(sceneBuffered)
            , logicHz(logicHz)
        {
            setupRendererPriority();
			resize(window.getClientWidth(), window.getClientHeight());

            std::cout << "frame buffer: " << resources.framebufferHandle << std::endl;
            std::cout << "depth buffer: " << resources.depthbufferHandle << std::endl;
        }

        ResourceManager& getResourceManager() { return resources; }

        void renderFrame(const Scene& scene, uint32_t frameIndex)
        {   
            auto projection = Mtx4::Perspective(
                scene.camera.fovRad, 
                viewport.computeAspectRatio(), 
                scene.camera.nearZ, 
                scene.camera.farZ
			);

            auto ctx = device.createContext();

            ctx.frameIndex = frameIndex;
            ctx.clearColor = Color::Grayscale(0.4f);
			ctx.model = scene.modelMatrix;
            ctx.view = scene.camera.calculateView();
            ctx.projection = projection;
            ctx.viewport = viewport;
            ctx.framebufferHandle = resources.framebufferHandle;
                
            device.clear(ctx);
            device.drawStaticTestMesh(ctx);
            
            device.present(resources.framebufferHandle);
        }

        void runLoop()
        {
            running.start();

			Scene scenePrevious;
			Scene sceneCurrent;
            Scene sceneInterpolated;

            TimeManager timerLogic(logicHz);
            TimeManager timer1Hz(1.0); // pro výpoèet FPS každou sekundu

            SceneInterpolationPair sceneHistory;

			uint32_t frameIndex = 0;
            while (running.isRunning())
            {

                {
                    if (uint32_t w, h; resizeRequest.consume(w, h))
                    {
                        this->resize(w, h);
                    }
                }

                if (sceneBuffered.update_reader()) {

                    sceneHistory.advance_and_get_current() = sceneBuffered.get_read_buffer();
                }

                timerLogic.tickAndFlush();

                sceneCurrent = sceneHistory.get_current();
                scenePrevious = sceneHistory.get_previous();

                double now = timerLogic.sinceStart();
                double lastTick = sceneCurrent.lastLogicTick;
                double t = (now - lastTick) / timerLogic.getFixedDelta();
				double tClamped = std::clamp(t, 0.0, 1.0);

				sceneInterpolated = Slerp(scenePrevious, sceneCurrent, static_cast<float>(tClamped));

                renderFrame(sceneInterpolated, frameIndex++);

                {
                    timer1Hz.tickAndDispatchAction([&](double dt) {
                        device.getWindow().setTitle(std::format(
                            "FPS: {:.0f} ||| 1% Low: {:.0f} ||| 0.1% Low: {:.0f} ||| Min/Max: {:.0f}/{:.0f} [Frame: {}]",
                            timerLogic.getFps(),
                            timerLogic.getLow1Percent(),
                            timerLogic.getLowPoint1Percent(),
                            timerLogic.getMaxFrameTimeFps(),
                            timerLogic.getMinFrameTimeFps(),
                            frameIndex
                        ));
                     });

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

        void setupRendererPriority() {
            // 1. Zvedneme prioritu celého procesu na "High".
            // To øíká Windows: "Tato aplikace je dùležitìjší než prohlížeè nebo Word."
            SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

            // 2. Nastavíme vlátnu rendereru "Time Critical" prioritu.
            // To znamená, že renderer dostane CPU pokaždé, když o nìj požádá,
            // a pøeruší témìø jakoukoli jinou èinnost v systému.
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
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

};
