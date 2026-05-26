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

        // tmp

        Mesh meshNet = Mesh::NetWave(80, 0.2f).applyTransformation(Mtx4::Identity().rotateX(GLibpp::Math::deg2rad(90.0f)).translate(-50.0f, -50.0f, 0.0f));

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
            ctx.view = scene.camera.calculateViewMatrix();
            ctx.projection = Mtx4::Perspective(fovRad, aspect, nearZ, farZ);
            ctx.viewport = viewport;
            ctx.framebufferHandle = resources.framebufferHandle;
                
            device.clear(ctx);

            {
                // Drawing commands

                // Ground - net
                Mesh meshNetAnim = Mesh::NetWave(60, 0.2f, frameIndex, 0.05f).applyTransformation(Mtx4::Identity().rotateX(GLibpp::Math::deg2rad(90.0f)).translate(-25.0f, -25.0f, 0.0f).scale(0.5f));
                device.drawMesh(ctx, meshNetAnim, Mtx4::Identity(), Color::Grayscale(0.3f), false);

                // Car
                device.drawMesh(ctx, scene.car.getMesh(), scene.car.getCarMatrix());

                // shpere
                device.drawMesh(ctx, Mesh::Icosan(1.0f, 2), scene.car.model.getTransformation(), Color::Grayscale(0.7f), false);
            
                // ICR
                device.drawMesh(ctx, Mesh::Cube(0.1f).applyTransformation(Mtx4::Scaling(0.01f, 8.0f, 0.01f)), scene.car.getIcrTransformation());

                // wheels
                device.drawMesh(ctx, scene.car.wheelFrontLeft.getMesh(), scene.car.getFrontLeft());
                device.drawMesh(ctx, scene.car.wheelFrontRight.getMesh(), scene.car.getFrontRight());
                device.drawMesh(ctx, scene.car.wheelBackLeft.getMesh(), scene.car.getBackLeft());
                device.drawMesh(ctx, scene.car.wheelBackRight.getMesh(), scene.car.getBackRight());

                // wheel axis
                device.drawMesh(ctx, Mesh::Cube(1.0f).applyTransformation(Mtx4::Scaling(12.0f, 0.01f, 0.01f)), scene.car.getFrontLeft());
                device.drawMesh(ctx, Mesh::Cube(1.0f).applyTransformation(Mtx4::Scaling(12.0f, 0.01f, 0.01f)), scene.car.getFrontRight());
                device.drawMesh(ctx, Mesh::Cube(1.0f).applyTransformation(Mtx4::Scaling(12.0f, 0.01f, 0.01f)), scene.car.getCarMatrix());

                // axis of local object spaces
                device.drawAxis(ctx, scene.car.getCarMatrix());
                device.drawAxis(ctx, Mtx4::Identity());
                device.drawAxis(ctx, scene.car.getFrontLeft().scale(scene.car.model.params.wheelRadius));
                device.drawAxis(ctx, scene.car.getFrontRight().scale(scene.car.model.params.wheelRadius));
                device.drawAxis(ctx, scene.car.getBackLeft().scale(scene.car.model.params.wheelRadius));
                device.drawAxis(ctx, scene.car.getBackRight().scale(scene.car.model.params.wheelRadius));
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

                //if (t < 0.0f || t > 1.0f) std::cout << t << std::endl;
				
                logicStateInterpolated.scene = Slerp(
                    logicStatePrevious.scene,
                    logicStateCurrent.scene, 
                    static_cast<float>(tClamped)
                );
                std::cout << "Previous" << std::endl << logicStatePrevious.scene.camera.calculateViewMatrix().toString() << std::endl << std::endl;
                std::cout << "Current" << std::endl << logicStateCurrent.scene.camera.calculateViewMatrix().toString() << std::endl << std::endl;
                std::cout << "Interpolated (t: " << t << ")" << std::endl << logicStateInterpolated.scene.camera.calculateViewMatrix().toString() << std::endl << "============" << std::endl;
                
                renderFrame(logicStateInterpolated.scene, ++frameIndex);

                timer1Hz.tickAndDispatchAction([&](double dt) {
                    device.getWindow().postMessageSetTitle(timer, frameIndex);


                    
                    

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
