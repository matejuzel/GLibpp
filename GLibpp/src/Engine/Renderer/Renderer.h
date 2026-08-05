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

        using LogicStateFramePair = ZeroAllocStateHistory<LogicState>;

        Device device;
        Viewport viewport;
        ResizeRequest resizeRequest;

        // kanonicke uloziste assetu - vlastni ho App, renderer je jen konzument/orchestrator
        ResourceManager& resources;

        // render targety vlastni Renderer - jsou to stavy render pipeline, ne assety
        typename Device::TargetHandle framebufferHandle;
        typename Device::TargetHandle depthbufferHandle;

        float logicHz;

        LogicStateBuffered& logicStateBuffered;
        RunState running;

    public:

        Renderer(WindowWin32& window, ResourceManager& resources, LogicStateBuffered& logicStateBuffered, float logicHz)
            : device(window)
			, viewport{ 0, 0, window.getClientWidth(), window.getClientHeight() }
			, resources(resources)
            , logicHz(logicHz)
            , logicStateBuffered(logicStateBuffered)
        {
            auto width = window.getClientWidth();
            auto height = window.getClientHeight();
            framebufferHandle = device.targetCreate(RenderTargetDescriptor::FramebufferRGBA32bit(width, height));
            depthbufferHandle = device.targetCreate(RenderTargetDescriptor::Depthbuffer24bit(width, height));

			resize(width, height);

            std::cout << "frame buffer: " << framebufferHandle << std::endl;
            std::cout << "depth buffer: " << depthbufferHandle << std::endl;
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
            ctx.framebufferHandle = framebufferHandle;
                
            device.clear(ctx);

            {
                // Drawing commands

                // Ground - dynamicka vlna: in-place mutace kanonickych dat (zero-alloc)
                // + re-upload zmeneneho range do residency backendu
                // (parametry vlny musi sedet s registraci v App::setupDemoResources)
                if (resources.meshInstanceIsValid(scene.renderables.gridWave))
                {
                    MeshHandle waveMesh = resources.meshInstanceGet(scene.renderables.gridWave).mesh;
                    MeshFactory::UpdateGridWave(resources.meshGetDynamic(waveMesh), 60, 0.2f, static_cast<float>(frameIndex), 0.05f);
                    device.meshUpdate(waveMesh, resources.meshGet(waveMesh));
                }
                drawInstance(ctx, scene.renderables.gridWave, Mtx4::Identity());

                // Car
                drawInstance(ctx, scene.renderables.carBody, scene.car.getCarMatrix());

                // shpere
                drawInstance(ctx, scene.renderables.icosphere, scene.car.model.getTransformation());

                // ICR
                drawInstance(ctx, scene.renderables.icrBeam, scene.car.getIcrTransformation());

                // wheels - jedna sdilena instance kreslena 4x s ruznymi world maticemi
                drawInstance(ctx, scene.renderables.wheel, scene.car.getFrontLeft());
                drawInstance(ctx, scene.renderables.wheel, scene.car.getFrontRight());
                drawInstance(ctx, scene.renderables.wheel, scene.car.getBackLeft());
                drawInstance(ctx, scene.renderables.wheel, scene.car.getBackRight());

                // axis of local object spaces
                device.drawAxis(ctx, scene.car.getCarMatrix());
                device.drawAxis(ctx, Mtx4::Identity());
                device.drawAxis(ctx, scene.car.getFrontLeft().scale(scene.car.model.params.wheelRadius));
                device.drawAxis(ctx, scene.car.getFrontRight().scale(scene.car.model.params.wheelRadius));
                device.drawAxis(ctx, scene.car.getBackLeft().scale(scene.car.model.params.wheelRadius));
                device.drawAxis(ctx, scene.car.getBackRight().scale(scene.car.model.params.wheelRadius));
            }

            device.present(framebufferHandle);
        }

        void runLoop()
        {
            LogicState logicStateInterpolated;
            LogicStateFramePair logicStateFramePair;

            TimeManager timer(logicHz, true);
            TimeManager timer1Hz(1.0); // pro výpočet FPS každou sekundu
			TimeManager timerSyncV(logicHz); // fallback pacing, pouzije se jen kdyz selze DwmFlush()

			uint32_t frameIndex = 0;

            // od ted je registrace resources zakazana - na data saha uz jen render vlakno
            resources.freeze();

            // upload point - render vlakno (u GL backendu tady bude aktivni context);
            // backend si z kanonickych dat postavi vlastni residency (kopie, offsety, VBO, ...)
            resources.meshForEach([&](MeshHandle h, const Mesh& m) { device.meshRegister(h, m); });

            running.start();

            while (running.isRunning())
            {

                {
                    // budouci seam: resources.uploadQueueConsume() - SPSC fronta pro runtime tvorbu
                    // resources z logickeho vlakna (skutecne add() do registru probehne az tady, na render vlakne)
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

    private:

        // kresli instanci pres handle; INVALID se tise preskoci
        // (napr. prvni framy, kdy triple buffer jeste drzi default-konstruovany stav)
        void drawInstance(const typename Device::Context& ctx, MeshInstanceHandle h, const Mtx4& world)
        {
            if (!resources.meshInstanceIsValid(h)) return;
            const MeshInstance& inst = resources.meshInstanceGet(h);
            device.drawMesh(ctx, inst.mesh, world * inst.localTransform, inst.color, inst.wireframe);
        }

        // Metoda resize(w, h) nesmi byt volana z jineho threadu
        void resize(uint32_t width, uint32_t height)
        {
            if (width == 0 || height == 0) return;
            // pozn.: pri selhani targetResize se handle prepise na TARGET_INVALID a target je nenavratne
            // ztraceny (znamy footgun, oprava mimo scope teto zmeny)
            framebufferHandle = device.targetResize(framebufferHandle, width, height);
            depthbufferHandle = device.targetResize(depthbufferHandle, width, height);
            viewport.resize(width, height);
        }

    };

};
