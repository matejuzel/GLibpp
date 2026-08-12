#pragma once

#include "RenderTargetDescriptor.h"
#include "Viewport.h"
#include "Color.h"
#include "DrawCommand.h"
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
#include "AtomicMailbox.h"
#include "StableRegistry.h"
#include "RunState.h"
#include "TimeManager.h"
#include "ZeroAllocStateHistory.h"
#include <iterator>
#include <utility>
#include "Mathematics.h"
#include "MeshFactory.h"

namespace GLibpp::Render {

    // pozadavek na resize z logickeho vlakna (WM_SIZE); do render vlakna
    // cestuje pres Core::AtomicMailbox - oba rozmery se prenaseji nedelitelne
    // jednim atomikem a novejsi pozadavek prepise starsi (posledni vyhrava).
    // Minimalizace (0x0) se pakuje na nulu = prazdna schranka; to nevadi,
    // resize(0,0) se stejne ignoruje.
    struct ResizeRequest {
        uint32_t width = 0;
        uint32_t height = 0;
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

        // ---- draw command stream ----
        // kapacita command listu; demo emituje ~21 commandu na frame, zbytek rezerva
        static constexpr size_t kDrawListCapacity = 256;

        using DrawCmd = DrawCommand<Device>;

        Device device;
        Viewport viewport;
        Core::AtomicMailbox<ResizeRequest> resizeRequest;

        // command list - plni ho build faze, prehrava submit faze (reuse, zadne alokace za behu)
        DrawList<Device, kDrawListCapacity> drawList;

        // kanonicke uloziste assetu - vlastni ho App, renderer je jen konzument/orchestrator
        Assets::ResourceManager& resources;

        // render targety vlastni Renderer - jsou to stavy render pipeline, ne assety
        typename Device::TargetHandle framebufferHandle;
        typename Device::TargetHandle depthbufferHandle;

        // capture textury (zrcadlo framebufferu + vizualizace hloubky):
        // residency targety textur ze sceny, resolvuji se lazy pri prvnim
        // framu s platnym handlem - residency existuje az po upload walku
        typename Device::TargetHandle fbCaptureTarget = Device::TARGET_INVALID;
        typename Device::TargetHandle depthCaptureTarget = Device::TARGET_INVALID;

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
            depthbufferHandle = device.targetCreate(RenderTargetDescriptor::Depthbuffer32F(width, height));

			resize(width, height);

            std::cout << "frame buffer: " << framebufferHandle << std::endl;
            std::cout << "depth buffer: " << depthbufferHandle << std::endl;
        }

        void renderFrame(const Scene& scene, uint32_t frameIndex)
        {
            // Ground - dynamicka vlna: in-place mutace kanonickych dat (zero-alloc)
            // + re-upload zmeneneho range do residency backendu; je to sprava resourcu,
            // ne draw command (dvirka: budouci UpdateMesh command)
            // (parametry vlny musi sedet s registraci v App::setupDemoResources)
            if (resources.meshInstanceIsValid(scene.renderables.gridWave))
            {
                Assets::MeshHandle waveMesh = resources.meshInstanceGet(scene.renderables.gridWave).mesh;
                Geometry::MeshFactory::UpdateGridWave(resources.meshGetDynamic(waveMesh), 60, 0.2f, static_cast<float>(frameIndex), 0.05f);
                device.meshUpdate(waveMesh, resources.meshGet(waveMesh));
            }

            auto ctx = device.createContext();
            ctx.frameIndex = frameIndex;

            drawList.reset();
            buildDrawList(scene);
            submitDrawList(ctx);

            // zaznam hotoveho snimku do capture textur - fbPanel/depthPanel
            // je samplují PRISTI frame (zivy framebuffer samplovat nejde,
            // clear na zacatku framu by predchozi obraz znicil)
            captureFrame(scene);

            device.present(framebufferHandle);
        }

        // kopie hotoveho framu: barva framebufferu 1:1 (zrcadlo vc. panelu
        // samotneho - Droste efekt) a hloubka jako grayscale (near svetla,
        // far cerna; dratene cary hloubku nezapisuji, takze na depth panelu
        // je jen plnena geometrie)
        void captureFrame(const Scene& scene)
        {
            if (fbCaptureTarget == Device::TARGET_INVALID)
                fbCaptureTarget = device.textureTargetGet(scene.renderables.fbTexture);
            if (depthCaptureTarget == Device::TARGET_INVALID)
                depthCaptureTarget = device.textureTargetGet(scene.renderables.depthTexture);

            // po resize okna se capture textury dotahnou na rozmer framebufferu,
            // jinak by kopie (guard na shodne rozmery) prestala prochazet a
            // zrcadlo zamrzlo; kanonicka TextureData zustava zamerne stale -
            // obsah je ciste residency zalezitost
            syncCaptureSize(fbCaptureTarget);
            syncCaptureSize(depthCaptureTarget);

            device.targetCopyColor(framebufferHandle, fbCaptureTarget);
            device.targetCopyDepthGray(depthbufferHandle, depthCaptureTarget);
        }

        void syncCaptureSize(typename Device::TargetHandle& handle)
        {
            if (handle == Device::TARGET_INVALID) return;

            const auto& d = device.targetGet(handle).descriptor;
            if (d.width == viewport.width && d.height == viewport.height) return;

            // pozn.: stejny footgun jako v resize() - pri selhani se handle
            // prepise na TARGET_INVALID a capture tim navzdy skonci
            handle = device.targetResize(handle, viewport.width, viewport.height);
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
            resources.textureForEach([&](Assets::TextureHandle h, const Assets::TextureData& t) { device.textureRegister(h, t); });

            running.start();

            while (running.isRunning())
            {

                {
                    // budouci seam: resources.uploadQueueConsume() - SPSC fronta pro runtime tvorbu
                    // resources z logickeho vlakna (skutecne add() do registru probehne az tady, na render vlakne)
                    if (ResizeRequest rq; resizeRequest.consume(rq))
                    {
                        this->resize(rq.width, rq.height);
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
            resizeRequest.set({ width, height });
        }

    private:

        // build faze (extract): interpolovana Scene -> linearni command list.
        // Vzajemne zakryti plnych trojuhelniku resi depth buffer, takze poradi
        // drawu uz neni nosne; drateny model a osy ale hloubku ignoruji (debug
        // overlay), tam poradi emitu porad rozhoduje. Budouci hierarchicky graf
        // nahradi prave tuhle metodu (flat uzly -> propagace worldu -> emit).
        void buildDrawList(const Scene& scene)
        {
            // stav framu - drawy nasleduji az za nim (viz poznamka o razeni v DrawCommand.h)
            drawList.setFramebuffer(framebufferHandle);
            drawList.setDepthbuffer(depthbufferHandle);
            drawList.setView(scene.camera.calculateViewMatrix());
            drawList.setProjection(Mtx4::Perspective(scene.camera.fovRad, viewport.computeAspectRatio(), scene.camera.nearZ, scene.camera.farZ));
            drawList.setViewport(viewport);
            drawList.clear(Color::Grayscale(0.4f));

            // world matice se pocitaji jednou a sdili je mesh i axis command
            const Mtx4 carM = scene.car.getCarMatrix();
            const Mtx4 wheelFL = scene.car.getFrontLeft();
            const Mtx4 wheelFR = scene.car.getFrontRight();
            const Mtx4 wheelBL = scene.car.getBackLeft();
            const Mtx4 wheelBR = scene.car.getBackRight();

            // --- pass 1: dratena zem Lambertem (prvni, aby ji plne objekty prekryly -
            //     cary ignoruji hloubku) ---
            drawList.setShader(FragmentShaderId::Lambert);
            drawList.drawMesh(Mtx4::Identity(), scene.renderables.gridWave);

            // --- pass 2: texturovane panely (demonstrace SetShader + SetTexture) ---
            // pozn.: plnoplosna texturovana zem neprosla - vyplneni ~poloviny okna
            // je v Debugu (/Od) nad rozpoctem framu (~30 FPS) nezavisle na poctu
            // trojuhelniku; proto texturu nesou male panely a vlna zustava dratena
            drawList.setShader(FragmentShaderId::Textured);
            drawList.setTexture(scene.renderables.panelTexture);
            drawList.drawMesh(Mtx4::Identity(), scene.renderables.texPanel);

            // zrcadlo: framebuffer minuleho framu (kopie na konci framu, viz
            // captureFrame) - obsahuje i panel samotny, takze zrcadlo v zrcadle
            drawList.setTexture(scene.renderables.fbTexture);
            drawList.drawMesh(Mtx4::Identity(), scene.renderables.fbPanel);

            // vizualizace hloubky minuleho framu (near svetla, far cerna)
            drawList.setTexture(scene.renderables.depthTexture);
            drawList.drawMesh(Mtx4::Identity(), scene.renderables.depthPanel);

            // --- pass 3: zbytek sceny plochym Lambertem ---
            drawList.setShader(FragmentShaderId::Lambert);

            // testovaci model nacteny z .obj (data/models); poradi vuci autu uz
            // nehraje roli - vzajemne zakryti resi depth buffer
            drawList.drawMesh(Mtx4::Identity(), scene.renderables.test);

            // Car
            drawList.drawMesh(carM, scene.renderables.carBody);

            // sphere (stejny world jako telo auta)
            drawList.drawMesh(carM, scene.renderables.icosphere);

            // ICR
            drawList.drawMesh(scene.car.getIcrTransformation(), scene.renderables.icrBeam);

            // wheels - jedna sdilena instance kreslena 4x s ruznymi world maticemi
            drawList.drawMesh(wheelFL, scene.renderables.wheel);
            drawList.drawMesh(wheelFR, scene.renderables.wheel);
            drawList.drawMesh(wheelBL, scene.renderables.wheel);
            drawList.drawMesh(wheelBR, scene.renderables.wheel);

            // osy lokalnich prostoru objektu (Mtx4::scale je mutujici builder -> skaluje se kopie)
            const float wheelR = scene.car.model.params.wheelRadius;
            drawList.drawAxis(carM);
            drawList.drawAxis(Mtx4::Identity());
            drawList.drawAxis(Mtx4(wheelFL).scale(wheelR));
            drawList.drawAxis(Mtx4(wheelFR).scale(wheelR));
            drawList.drawAxis(Mtx4(wheelBL).scale(wheelR));
            drawList.drawAxis(Mtx4(wheelBR).scale(wheelR));
        }

        // submit faze: linearni prehrani command listu do Device pres dispatch tabulku
        void submitDrawList(typename Device::Context& ctx)
        {
            for (const DrawCmd& cmd : drawList)
            {
                kDispatch[static_cast<size_t>(cmd.kind)](*this, ctx, cmd);
            }
        }

        // ---- executory draw commandu ----
        // jednotna signatura (Renderer&, Context&, command) kvuli dispatch tabulce

        static void execDrawMesh(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            // INVALID se tise preskoci
            // (napr. prvni framy, kdy triple buffer jeste drzi default-konstruovany stav)
            if (!r.resources.meshInstanceIsValid(c.drawMesh.instance)) return;
            const Geometry::MeshInstance& inst = r.resources.meshInstanceGet(c.drawMesh.instance);
            r.device.drawMesh(ctx, inst.mesh, c.drawMesh.world * inst.localTransform, inst.color, inst.wireframe);
        }

        static void execDrawAxis(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            r.device.drawAxis(ctx, c.drawAxis.world);
        }

        static void execSetView(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            ctx.setView(c.matrix.matrix);
        }

        static void execSetProjection(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            ctx.setProjection(c.matrix.matrix);
        }

        static void execSetViewport(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            ctx.setViewport(c.viewport.viewport);
        }

        static void execSetFramebuffer(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            ctx.framebufferHandle = c.target.target;
        }

        static void execSetDepthbuffer(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            ctx.depthbufferHandle = c.target.target;
        }

        static void execClear(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            ctx.clearColor = c.clear.color;
            r.device.clear(ctx);
        }

        static void execSetShader(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            ctx.fragmentShader = c.shader.shader;
        }

        static void execSetTexture(Renderer& r, typename Device::Context& ctx, const DrawCmd& c)
        {
            ctx.texture = c.texture.texture;
        }

        using ExecuteFn = void (*)(Renderer&, typename Device::Context&, const DrawCmd&);

        // dispatch tabulka: index = DrawCmd::Kind, poradi radku musi sedet s enumem
        static constexpr ExecuteFn kDispatch[] = {
            &Renderer::execDrawMesh,
            &Renderer::execDrawAxis,
            &Renderer::execSetView,
            &Renderer::execSetProjection,
            &Renderer::execSetViewport,
            &Renderer::execSetFramebuffer,
            &Renderer::execSetDepthbuffer,
            &Renderer::execClear,
            &Renderer::execSetShader,
            &Renderer::execSetTexture,
        };
        static_assert(std::size(kDispatch) == static_cast<size_t>(DrawCmd::Kind::Count),
            "dispatch tabulka musi pokryvat vsechny druhy commandu");

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
