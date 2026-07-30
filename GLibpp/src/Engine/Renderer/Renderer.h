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
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
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
#include "MeshFactory.h"

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
        ResourceManager::MeshHandle mshHandle00 = resources.meshRegister(MeshFactory::CreateCube(1.0f));
        ResourceManager::MeshInstanceHandle mshInstHandle00 = resources.meshInstanceRegister(mshHandle00, Mtx4::Identity());
        ResourceManager::MeshInstanceHandle mshInstHandle01 = resources.meshInstanceRegister(mshHandle00, Mtx4::Identity().rotateY(0.3f));
        ResourceManager::MeshInstanceHandle mshInstHandle02 = resources.meshInstanceRegister(mshHandle00, Mtx4::Identity().rotateY(0.6f));
        ResourceManager::MeshInstanceHandle mshInstHandle03 = resources.meshInstanceRegister(mshHandle00, Mtx4::Identity().rotateY(0.9f));

        // tmp

        Mesh meshGrid = MeshFactory::CreateGrid(80, 0.2f).applyTransformation(Mtx4::Identity().rotateX(GLibpp::Math::deg2rad(90.0f)).translate(-50.0f, -50.0f, 0.0f));

        // staticke meshe demo sceny - vygenerovane jednou, ne v kazdem framu
        Mesh meshIcosphere = MeshFactory::CreateIcosphere(1.0f, 4);
        Mesh meshGridWave = MeshFactory::CreateGridWave(60, 0.2f, 0.0f, 0.05f);
        Mtx4 gridWaveModel = Mtx4::Identity().rotateX(GLibpp::Math::deg2rad(90.0f)).translate(-25.0f, -25.0f, 0.0f).scale(0.5f);
        Mesh meshIcrBeam = MeshFactory::CreateCube(0.1f).applyTransformation(Mtx4::Scaling(0.01f, 8.0f, 0.01f));
        Mesh meshAxleBeam = meshAxleBeam;

    public:

        Renderer(WindowWin32& window, LogicStateBuffered& logicStateBuffered, float logicHz)
            : device(window)
			, resources(device)
			, viewport{ 0, 0, window.getClientWidth(), window.getClientHeight() }
            , logicStateBuffered(logicStateBuffered)
            , logicHz(logicHz)
        {
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
            ctx.setView(scene.camera.calculateViewMatrix());
            ctx.setProjection(Mtx4::Perspective(fovRad, aspect, nearZ, farZ));
            ctx.setViewport(viewport);
            ctx.framebufferHandle = resources.framebufferHandle;
                
            device.clear(ctx);

            {
                // Drawing commands

                // Ground - net (vlna se aktualizuje in-place, transformace jde pres model matici)
                MeshFactory::UpdateGridWave(meshGridWave, 60, 0.2f, static_cast<float>(frameIndex), 0.05f);
                device.drawMesh(ctx, meshGridWave, gridWaveModel, Color::Grayscale(0.3f), true);

                // Car
                device.drawMesh(ctx, scene.car.getMesh(), scene.car.getCarMatrix());

                // shpere
                device.drawMesh(ctx, meshIcosphere, scene.car.model.getTransformation(), Color::Grayscale(0.7f), true);
            
                // ICR
                device.drawMesh(ctx, meshIcrBeam, scene.car.getIcrTransformation());

                // wheels
                device.drawMesh(ctx, scene.car.wheelFrontLeft.getMesh(), scene.car.getFrontLeft());
                device.drawMesh(ctx, scene.car.wheelFrontRight.getMesh(), scene.car.getFrontRight());
                device.drawMesh(ctx, scene.car.wheelBackLeft.getMesh(), scene.car.getBackLeft());
                device.drawMesh(ctx, scene.car.wheelBackRight.getMesh(), scene.car.getBackRight());

                // wheel axis
                device.drawMesh(ctx, meshAxleBeam, scene.car.getFrontLeft());
                device.drawMesh(ctx, meshAxleBeam, scene.car.getFrontRight());
                device.drawMesh(ctx, meshAxleBeam, scene.car.getCarMatrix());

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
            TimeManager timer1Hz(1.0); // pro výpočet FPS každou sekundu
			TimeManager timerSyncV(logicHz); // fallback pacing, pouzije se jen kdyz selze DwmFlush()

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

                double t;
                if (false) 
                {
                    t = (timer.sinceStart() - logicStateCurrent.tickInfo.lastLogicTick) / timer.getFixedDelta();
                }
                else
                {
                    // 1. Zjistíme přesné časové značky obou stavů z Triple Bufferu
                    double timePrev = logicStatePrevious.tickInfo.lastLogicTick;
                    double timeCurr = logicStateCurrent.tickInfo.lastLogicTick;

                    // 2. Skutečný časový rozdíl mezi stavy (chráníme proti dělení nulou)
                    double stateDelta = timeCurr - timePrev;
                    if (stateDelta <= 0.0001) {
                        stateDelta = timer.getFixedDelta(); // fallback
                    }

                    // 3. Vypočítáme Vizuální Čas = aktuální čas mínus jedno logické okno
                    // Tím se vždy držíme bezpečně MEZI timePrev a timeCurr
                    // faktor 1.1 = mala rezerva na latenci publishe (pollEvents + updateLogic),
                    // aby t tesne nepretekalo pres 1.0
                    double visualTime = timer.sinceStart() - timer.getFixedDelta() * 1.1;

                    // 4. Výpočet alfa na základě skutečného rozpětí
                    t = (visualTime - timePrev) / stateDelta;
                }
                // logujeme jen skutecne zaseky logiky (chybejici stav > pul ticku);
                // drobne pretece t se tise clampne - cout na render vlakne je drahy
                if (t >= 1.5) std::cout << "Zaskub: t = " << t << std::endl;
                double tClamped = std::clamp(t, 0.0, 1.0);

                logicStateInterpolated.scene = Slerp(
                    logicStatePrevious.scene,
                    logicStateCurrent.scene,
                    static_cast<float>(tClamped)
                );

                renderFrame(logicStateInterpolated.scene, ++frameIndex);

                timer1Hz.tickAndDispatchAction([&](double dt) {
                    device.getWindow().postMessageSetTitle(timer, frameIndex);
                });

                // Synchronizace s kompozitorem: DwmFlush() blokuje do dalsi kompozice (vblank),
                // takze snimky jdou na obrazovku v pravidelnem rytmu refresh rate monitoru
                if (FAILED(DwmFlush()))
                {
                    // fallback kdyz DWM neni k dispozici: pevny pacing na logicHz
                    timerSyncV.tick();
                    timerSyncV.waitUntilNextStep();
                    timerSyncV.dispatchAction([](double dt) {});
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

    };

};
