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

        Device device;
        
        Viewport viewport;
        
        RunState running;
        ResizeRequest resizeRequest;

        ResourceManager resources;
       

        DoubleBuffer<std::pair<Scene, Scene>> sceneBuffer;

        float logicHz;

        std::atomic<double> lastLogicTick = { 0.0 };

    public:

        // V DoubleBufferu máme: std::pair<Camera, Camera> 
        // first  = Aktuální (Current)
        // second = Pøedchozí (Previous)

        void updateLastLogicTick(double dt)
        {
            lastLogicTick.store(dt, std::memory_order_relaxed);
        }

        double getLastLogicTick() const
        {
            return lastLogicTick.load(std::memory_order_relaxed);
		}

        void updateScene(const Scene& newScene)
        {
            // 1. Získáme referenci na back buffer, do kterého budeme pøipravovat nová data
            auto& back = sceneBuffer.writeBuffer();

            // 2. Získáme referenci na to, co se právì teï vykresluje (front buffer)
            const auto& front = sceneBuffer.readBuffer();

            // 3. LOGIKA POSUNU:
            // To, co bylo v minulém snímku "aktuální", je v tomto snímku "pøedchozí"
            back.second = front.first;

            // To, co nám pøišlo teï, je "aktuální"
            back.first = newScene;

            // 4. Publikujeme (pøehodíme indexy)
            sceneBuffer.publish();
        }


		ResourceManager& getResourceManager() { return resources; }

        Renderer(WindowWin32& window, float logicHz)
            : device(window)
			, resources(device)
			, viewport{ 0, 0, window.getClientWidth(), window.getClientHeight() }
            , logicHz(logicHz)
        {
            setupRendererPriority();
			resize(window.getClientWidth(), window.getClientHeight());

            std::cout << "frame buffer: " << resources.framebufferHandle << std::endl;
            std::cout << "depth buffer: " << resources.depthbufferHandle << std::endl;
        }

        void renderFrame(double dt, uint32_t frameIndex)
        {   
            {
                auto ctx = device.createContext();

                dt = std::clamp(static_cast<float>(dt), 0.0f, 1.0f);

				Camera camera = Slerp(sceneBuffer.readBuffer().second.camera, sceneBuffer.readBuffer().first.camera, dt);
                Mtx4 matrix0 = sceneBuffer.readBuffer().second.modelMatrix; // @test bez interpolace
                Mtx4 matrix1 = sceneBuffer.readBuffer().first.modelMatrix; // @test bez interpolace
				
				Mtx4 matrix = Mtx4::Slerp(matrix0, matrix1, dt);
                
				ctx.model = matrix;

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

        void runLoop()
        {
		    
            running.start();

            TimeManager timerLogic(logicHz);
            TimeManager timer1Hz(1.0); // pro výpoèet FPS každou sekundu

			uint32_t frameIndex = 0;
            while (running.isRunning())
            {

                {
                    if (uint32_t w, h; resizeRequest.consume(w, h))
                    {
                        this->resize(w, h);
                    }
                }

                timerLogic.tickAndFlush();

                double now = timerLogic.sinceStart();
                double lastTick = this->getLastLogicTick();
                float t = static_cast<float>((now - lastTick) / timerLogic.getFixedDelta());

                renderFrame(t, frameIndex++);

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
