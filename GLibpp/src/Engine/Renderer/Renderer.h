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

#include "ResourceManager.h"
#include "Scene.h"
#include "Mesh.h"
#include "StableRegistry.h"
#include "AssetRegistry.h"
#include "RunState.h"
#include "TimeManager.h"
#include "ZeroAllocStateHistory.h"
#include <utility>
#include "Mathematics.h"

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
        using LogicStateFramePair = ZeroAllocStateHistory<LogicState>;

        Device device;
        Viewport viewport;
        ResizeRequest resizeRequest;
        ResourceManager resources;

        float logicHz;

        LogicStateBuffered& logicStateBuffered;
        RunState running;

        // docasne zatim takhle
        ResourceManager::MeshHandle mshHandle00 = resources.meshRegister(Mesh::Cube(1.0f));
        ResourceManager::MeshInstanceHandle mshInstHandle00 = resources.meshInstanceRegister(mshHandle00, Mtx4::Identity());
        ResourceManager::MeshInstanceHandle mshInstHandle01 = resources.meshInstanceRegister(mshHandle00, Mtx4::Identity().rotateY(0.3f));
        ResourceManager::MeshInstanceHandle mshInstHandle02 = resources.meshInstanceRegister(mshHandle00, Mtx4::Identity().rotateY(0.6f));
        ResourceManager::MeshInstanceHandle mshInstHandle03 = resources.meshInstanceRegister(mshHandle00, Mtx4::Identity().rotateY(0.9f));
    public:

        Renderer(WindowWin32& window, LogicStateBuffered& logicStateBuffered, float logicHz)
            : device(window)
			, resources(device)
			, viewport{ 0, 0, window.getClientWidth(), window.getClientHeight() }
            , logicStateBuffered(logicStateBuffered)
            , logicHz(logicHz)
        {
            setupRendererPriority();
			resize(window.getClientWidth(), window.getClientHeight());

            std::cout << "frame buffer: " << resources.framebufferHandle << std::endl;
            std::cout << "depth buffer: " << resources.depthbufferHandle << std::endl;
        }

        void renderFrame(const Scene& scene, uint32_t frameIndex)
        {   
			auto fovRad = scene.camera.fovRad;
			auto aspect = viewport.computeAspectRatio();
			auto nearZ = scene.camera.nearZ;
			auto farZ = scene.camera.farZ;

            auto ctx = device.createContext();

            ctx.frameIndex = frameIndex;
            ctx.clearColor = Color::Grayscale(0.4f);
			ctx.model = scene.modelMatrix;
            ctx.view = scene.camera.calculateView();
            ctx.projection = Mtx4::Perspective(fovRad, aspect, nearZ, farZ);
            ctx.viewport = viewport;
            ctx.framebufferHandle = resources.framebufferHandle;
                
            device.clear(ctx);

            {
                uint32_t segments = 16;
                float radius = 0.5f;

                // scene.matrixVehicle * wheelSteer * wheelRoll * wheelOffset;
                device.drawMesh(ctx, Mesh::Cylinder(radius, 0.3, segments), scene.matrixVehicle * scene.matrixWheel01);
                device.drawMesh(ctx, Mesh::Cylinder(radius, 0.3, segments), scene.matrixVehicle * scene.matrixWheel02);
                device.drawMesh(ctx, Mesh::Cylinder(radius, 0.3, segments), scene.matrixVehicle * scene.matrixWheel03);
                device.drawMesh(ctx, Mesh::Cylinder(radius, 0.3, segments), scene.matrixVehicle * scene.matrixWheel04);
            }
            
            //device.drawMesh(ctx, Mesh::Cylinder(0.5f, 0.2, 8), scene.modelMatrix * scene.modelMatrix2);

            if (0)
            for (int i = 0; i < 8; ++i) 
            {
                device.drawStaticTestMesh(ctx);
                ctx.model.rotateZ(GLibpp::Math::deg2rad(10.0f));
            }
            

            device.present(resources.framebufferHandle);
        }

        void runLoop()
        {
            LogicState logicStateInterpolated;
            LogicStateFramePair logicStateFramePair;

            TimeManager timer(logicHz, true);
            TimeManager timer1Hz(1.0); // pro výpoèet FPS každou sekundu
			TimeManager timerSyncV(60.0);

			uint32_t frameIndex = 0;            

            running.start();

            while (running.isRunning())
            {

                {
                    if (uint32_t w, h; resizeRequest.consume(w, h))
                    {
                        this->resize(w, h);
                    }
                }

                if (logicStateBuffered.update_reader()) 
                {
                    // z tripple bufferu (App -> Renderer) si vezmeme posledni neprecteny stav a 
                    // ulozime ho do logicStateFramePair (aktualizace puvodni current posuneme na previous a ulozime novy current)
                    logicStateFramePair.advance_and_load_current(logicStateBuffered.get_read_buffer());
                }

                auto& logicStateCurrent = logicStateFramePair.get_current();
                auto& logicStatePrevious = logicStateFramePair.get_previous();

                timer.tickAndFlush();

                double t = (timer.sinceStart() - logicStateCurrent.tickInfo.lastLogicTick) / timer.getFixedDelta();
				double tClamped = std::clamp(t, 0.0, 1.0);
				
                logicStateInterpolated.scene = Slerp(
                    logicStatePrevious.scene,
                    logicStateCurrent.scene, 
                    static_cast<float>(tClamped)
                );

                
                renderFrame(logicStateInterpolated.scene, ++frameIndex);

                timer1Hz.tickAndDispatchAction([&](double dt) {
                    device.getWindow().setTitle(timer, frameIndex);
                });
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

        ResourceManager& getResourceManager()
        {
            return resources;
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

        void setupRendererPriority()
        {
            // Zvedneme prioritu JEN aktuálního vlákna renderu
            // z NORMAL na ABOVE_NORMAL.
            // To typicky zlepší stabilitu frameratu, aniž by to nièilo systém.
            HANDLE thread = GetCurrentThread();

            if (!SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL)) {
                // Volitelné: lognout chybu, ale nepanikaøit.
                // std::cerr << "Failed to set thread priority\n";
            }
        }

    };

};
