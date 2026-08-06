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

namespace GLibpp::Render {

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

        // ---- ladeni snapshot interpolace ----
        // rezerva: o kolik logickych tiku se vizualni cas drzi za simulaci.
        // Vetsi hodnota = odolnost proti pozde prichozimu publishi (t > 1 ->
        // opakovany snimek = mikrozaskub); cena = (k - 1) x 16,7 ms latence navic.
        static constexpr double kInterpDelayTicks = 2.0;

        // hloubka okna historie stavu: (N - 1) tiku musi pokryt rezervu, aby vzdy
        // existoval par stavu obklopujici visualTime -> strop(rezervy) + 1.
        // Odvozuje se automaticky - pri zmene kInterpDelayTicks se prizpusobi.
        static constexpr size_t kInterpHistoryDepth =
            static_cast<size_t>(kInterpDelayTicks)
            + ((kInterpDelayTicks > static_cast<double>(static_cast<size_t>(kInterpDelayTicks))) ? 1u : 0u) // strop
            + 1u;
        static_assert(static_cast<double>(kInterpHistoryDepth - 1) >= kInterpDelayTicks,
            "okno historie musi pokryvat interpolacni rezervu");

        using LogicStateHistory = Core::ZeroAllocStateHistory<LogicState, kInterpHistoryDepth>;

        Device device;
        Viewport viewport;
        ResizeRequest resizeRequest;

        // kanonicke uloziste assetu - vlastni ho App, renderer je jen konzument/orchestrator
        Assets::ResourceManager& resources;

        // render targety vlastni Renderer - jsou to stavy render pipeline, ne assety
        typename Device::TargetHandle framebufferHandle;
        typename Device::TargetHandle depthbufferHandle;

        float logicHz;

        BufferedLogicState& logicStateBuffered;
        Core::RunState running;

    public:

        Renderer(Platform::WindowWin32& window, Assets::ResourceManager& resources, BufferedLogicState& logicStateBuffered, float logicHz)
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
                    Assets::MeshHandle waveMesh = resources.meshInstanceGet(scene.renderables.gridWave).mesh;
                    Geometry::MeshFactory::UpdateGridWave(resources.meshGetDynamic(waveMesh), 60, 0.2f, static_cast<float>(frameIndex), 0.05f);
                    device.meshUpdate(waveMesh, resources.meshGet(waveMesh));
                }
                drawInstance(ctx, scene.renderables.gridWave, Mtx4::Identity());

                // testovaci model nacteny z .obj (data/models) - kresleny pred autem,
                // aby auto bez depth bufferu zustalo viditelne pri prujezdu kolem
                drawInstance(ctx, scene.renderables.test, Mtx4::Identity());

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
            LogicStateHistory logicStateHistory;

            // diagnostika clampu interpolacni alfy (1 Hz vypis, jen kdyz k necemu doslo)
            uint32_t interpClampHi = 0; // t > 1: stav prisel pozde, snimek se opakuje
            uint32_t interpClampLo = 0; // t < 0: vizualni cas pred prev, hraje se prev

            Core::TimeManager timer(logicHz, true);
            Core::TimeManager timer1Hz(1.0); // pro výpočet FPS každou sekundu
			Core::TimeManager timerSyncV(logicHz); // fallback pacing, pouzije se jen kdyz selze DwmFlush()

			uint32_t frameIndex = 0;

            // od ted je registrace resources zakazana - na data saha uz jen render vlakno
            resources.freeze();

            // upload point - render vlakno (u GL backendu tady bude aktivni context);
            // backend si z kanonickych dat postavi vlastni residency (kopie, offsety, VBO, ...)
            resources.meshForEach([&](Assets::MeshHandle h, const Geometry::Mesh& m) { device.meshRegister(h, m); });

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
                    // z tripple bufferu (App -> Renderer) si vezmeme posledni neprecteny stav
                    // a zaradime ho do ringu historie (nejstarsi stav vypadne)
                    logicStateHistory.advance_and_load_current(logicStateBuffered.get_read_buffer());
                }

                timer.tickAndFlush();

                // 1. Vizualni cas = aktualni cas minus rezerva (viz kInterpDelayTicks)
                double visualTime = timer.sinceStart() - timer.getFixedDelta() * kInterpDelayTicks;

                // 2. Snapshot interpolace: z historie vybereme dvojici stavu, ktera
                //    visualTime skutecne obklopuje. Zaciname u nejnovejsiho paru; kdyz je
                //    visualTime starsi nez starsi clen paru, posuneme se o krok do minulosti.
                //    Mimo okno historie (start, dlouhy zasek) se t nize clampne.
                const LogicState* older = &logicStateHistory.get(1);
                const LogicState* newer = &logicStateHistory.get(0);
                for (size_t age = 1; age + 1 < logicStateHistory.capacity(); ++age)
                {
                    if (visualTime >= logicStateHistory.get(age).tickInfo.lastLogicTick) break;
                    older = &logicStateHistory.get(age + 1);
                    newer = &logicStateHistory.get(age);
                }

                // 3. Casove znacky vybraneho paru (ochrana proti deleni nulou)
                double timeOlder = older->tickInfo.lastLogicTick;
                double stateDelta = newer->tickInfo.lastLogicTick - timeOlder;
                if (stateDelta <= 0.0001) {
                    stateDelta = timer.getFixedDelta(); // fallback (start, duplicitni stavy)
                }

                // 4. Vypocet alfa na zaklade skutecneho rozpeti vybraneho paru
                double t = (visualTime - timeOlder) / stateDelta;
                // logujeme jen skutecne zaseky logiky (chybejici stav > pul ticku);
                // drobne pretece t se tise clampne - cout na render vlakne je drahy
                if (t >= 1.5) std::cout << "Zaskub: t = " << t << std::endl;
                if (t > 1.0) ++interpClampHi; else if (t < 0.0) ++interpClampLo;
                double tClamped = std::clamp(t, 0.0, 1.0);

                logicStateInterpolated.scene = Slerp(
                    older->scene,
                    newer->scene,
                    static_cast<float>(tClamped)
                );

                renderFrame(logicStateInterpolated.scene, ++frameIndex);

                timer1Hz.tickAndDispatchAction([&](double dt) {
                    device.getWindow().postMessageSetTitle(timer, frameIndex);
                    if (interpClampHi || interpClampLo) {
                        std::cout << "Interp clamp/s: t>1: " << interpClampHi << " | t<0: " << interpClampLo << std::endl;
                        interpClampHi = 0; interpClampLo = 0;
                    }
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
        void drawInstance(const typename Device::Context& ctx, Assets::MeshInstanceHandle h, const Mtx4& world)
        {
            if (!resources.meshInstanceIsValid(h)) return;
            const Geometry::MeshInstance& inst = resources.meshInstanceGet(h);
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
