#pragma once
#include <memory>
#include <atomic>
#include <thread>
#include <random>

#include "WindowWin32.h"

#include "Mathematics.h"
#include "Mtx4.h"
#include "Vec4.h"
#include "Quaternion.h"
#include "Mesh.h"
#include "MeshFactory.h"
#include "ModelImporter.h"
#include "ImageLoaderWin32.h"
#include <fstream>

#include "Scene.h"
#include "Camera.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "RunState.h"
#include "RenderTargetDescriptor.h"
#include "TimeManager.h"
#include "Input.h"

#define RENDER_BACKEND_DIB

#if defined(RENDER_BACKEND_DIB)
#include "DeviceDIB.h"
using RenderDevice = GLibpp::Render::DeviceDIB;
#else
#error "Neni definovan backend!"
#endif

// aliasy do engine namespacu - App je listova aplikacni vrstva, muze si dovolit kratka jmena
using GLibpp::Platform::WindowWin32;
using GLibpp::Core::TimeManager;
using GLibpp::Core::RunState;
using GLibpp::Core::KeyMap;
using GLibpp::Geometry::Camera;
using GLibpp::Geometry::MeshFactory;
using GLibpp::Render::Color;

/*
Implementovat nekdy v budoucnu:
	Origin Rebasing
        pri rozsahlem svete a velkym offsetu kamery se zacne projevovat snizujicici se presnost floatu.
        Resenim je Origin Rebasing, ktery pravidelne posouva svet tak aby kamera byla co nejblize k originu.
*/

class App {
private:

    using RendererEngine = GLibpp::Render::Renderer<RenderDevice>;

    std::unique_ptr<WindowWin32> window;

    // kanonicke uloziste assetu (meshe, instance) - obsah vlastni App, renderer dostava referenci
    // deklarovano PRED rendererem: renderer drzi referenci, musi tedy zaniknout driv
    GLibpp::Assets::ResourceManager resources;

	//std::unique_ptr<RendererEngine> renderer; // obecny renderer, ktery pouziva RenderDevice (DIB, Stencil, ...), ktery se zvoli definici makra RENDER_BACKEND_XXX
    std::unique_ptr<GLibpp::Render::Renderer<GLibpp::Render::DeviceDIB>> renderer; // pro vyvoj pouzijeme takhle explicitne, kvuli napovidani v IDE...

    bool fullscreen = false;
    RunState running;

    GLibpp::Core::Input input;

    float logicHz = 60;

    void requireWindowInitialized() const {
        if (window.get() == nullptr) {
            throw std::runtime_error("Window is not initialized");
        }
	}

    BufferedLogicState logicStateBuffered;

public:

    App() = default;
	~App() = default;

	void setFullscreenMode(bool fullscreen) {
		requireWindowInitialized();
		window->setFullscreenMode(fullscreen);
    }

    void initialize(uint32_t width, uint32_t height, const std::wstring& preferedDisplayName = L"")
    {

        if (0) {

            float num = 0.5f;

            std::cout << GLibpp::Math::reciprocal_debug(num, 8) << std::endl;
            std::cout << 1.0f / num << std::endl;

            exit(0);
        }

        if (false)
        {
			Quaternion q1 = Quaternion::FromEuler(0.0f, GLibpp::Math::deg2rad(45.0f), 0.0f);
            auto q2 = q1;
            q2.rotateX(0.5f);

            std::cout << q1.toMatrix() << std::endl;
            std::cout << q2.toMatrix() << std::endl;

            std::cout << Slerp(q1, q2, 0.5f).toMatrix() << std::endl;

            exit(0);
        }


        {
            // WINDOW
            window = std::make_unique<WindowWin32>(width, height, false);

            if (!window->build(preferedDisplayName)) {
                throw std::runtime_error("Failed to create window");
            }

            if (fullscreen)
            {
                window->removeOverlapProperty();
                window->resizeWindowToFillScreen();
                window->hideCursor();
            }

            window->setOnCloseCallback([this]() {
                running.stop();
            });

            window->setKeyCallback([this](KeyMap key, bool pressed) {
                onKeyCallback(key, pressed);
            });

            window->setOnResizeCallback([this](uint32_t width, uint32_t height) {
                if (renderer)
                {
                    renderer->resizeRequestSet(width, height);
                    std::cout << "resized: " << width << " ; " << height << std::endl;
                }
            });

            window->registerRawInputDevices();
        }

        {
            // RENDERER
            renderer = std::make_unique<RendererEngine>(*window, resources, logicStateBuffered, logicHz);
        }

	}

    void lagTest(int ms)
    {
        // Simulace náročné logiky, která trvá ms milisekund
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		std::cout << "Lag test: Simulated work for " << ms << " ms" << std::endl;
	}

    void onKeyCallback(KeyMap key, bool pressed)
    {

        input.keyboard.setKeyState(static_cast<unsigned char>(key), pressed);

        if (key == KeyMap::KEY_ESC && pressed) {
            running.stop();
        }
	}

    void updateLogic(float dt, Scene& scene)
    {

        bool flag = true;
        if (input.keyboard.isDown(KeyMap::KEY_UP)) {
            scene.car.speedUp(3 * dt);
            flag = false;
        }

        if (input.keyboard.isDown(KeyMap::KEY_DOWN)) {
            scene.car.speedUp(-2 * dt);
            flag = false;
        }

        // pasivni dojezd ~0.6 m/s^2, skalovano dt jako sousedni vstupy
        // (drive 0.01/tick = zavisle na logicHz; pri 60 Hz je chovani shodne)
        if (flag) scene.car.speedDown(0.6f * dt);


        scene.car.run(dt);


        bool flagResetSteer = true;
        if (input.keyboard.isDown(KeyMap::KEY_LEFT)) {
            scene.car.steerFrontWheels(dt * 0.5f);
            flagResetSteer = false;
        }

        if (input.keyboard.isDown(KeyMap::KEY_RIGHT)) {
            scene.car.steerFrontWheels(-dt * 0.5f);
            flagResetSteer = false;
        }

        if (flagResetSteer) scene.car.steerFrontWheelsReset(dt);

        updateFollowCamera(scene, dt);
    }

    // follow kamera - drzi se za autem podle jeho headingu a kouka na nej;
    // pozice se dotahuje exponencialnim tlumenim (frame-rate nezavisle),
    // takze se kamera v zatacce prijemne opozdi; cil je rigidni - auto je vzdy v zaberu
    void updateFollowCamera(Scene& scene, float dt)
    {
        constexpr float followDistance = 12.0f; // za autem (podel -forward)
        constexpr float followHeight   = 5.0f;  // vyska kamery nad pozici auta
        constexpr float lookAtHeight   = 1.5f;  // cil mirne nad podvozkem
        constexpr float stiffness      = 2.0f;  // rychlost dotahovani pozice [1/s]

        Vec4 carPos = scene.car.model.getPosition();
        Vec4 forward = scene.car.model.getHeading() * Vec4(0.0f, 0.0f, 1.0f, 0.0f);

        Vec4 desired = carPos - forward * followDistance + Vec4(0.0f, followHeight, 0.0f, 0.0f);

        float alpha = 1.0f - std::exp(-stiffness * dt);
        Vec4 newPos = Lerp(scene.camera.position, desired, alpha);

        scene.camera.lookAtFrom(newPos, carPos + Vec4(0.0f, lookAtHeight, 0.0f, 0.0f));
    }

    // registrace demo geometrie do ResourceManageru - MUSI probehnout pred startem
    // render vlakna (registry neni thread-safe); Scene dostane jen handly instanci
    void setupDemoResources(Scene& scene)
    {
        const auto& p = scene.car.model.params;

        // vsechna 4 kola jsou geometricky shodna -> jeden sdileny mesh + jedna instance
        auto wheelMesh = resources.meshRegister(MeshFactory::CreateCylinder(p.wheelRadius, 0.4f, 12).applyTransformation(Mtx4::RotationZ(3.14159f / 2.0f)));
        auto bodyMesh = resources.meshRegister(MeshFactory::CreateCylinder(1.0f, 6, 16).applyTransformation(Mtx4::RotationX(3.14159f / 2.0f) * Mtx4::Translation(0.0f, p.wheelBase * 0.5f, 0.0f)));
        auto sphereMesh = resources.meshRegister(MeshFactory::CreateIcosphere(1.0f, 4));
        auto waveMesh = resources.meshRegister(MeshFactory::CreateGridWave(60, 0.2f, 0.0f, 0.05f));

        // texturovany panel - maly demonstracni objekt pro fragment shader Textured;
        // plnoplosna texturovana zem neprosla: vyplneni ~poloviny okna je v Debugu
        // (/Od) nad rozpoctem framu (~30 FPS), nezavisle na poctu trojuhelniku
        auto panelMesh = resources.meshRegister(MeshFactory::CreateQuad(4.0f));

        // ram zrcadla: vnitrni hrana liciuje s okrajem panelu (4.3 - 2*0.15 = 4.0)
        auto panelFrameMesh = resources.meshRegister(MeshFactory::CreateQuadFrame(4.3f, 0.15f));
        auto icrMesh = resources.meshRegister(MeshFactory::CreateCube(0.1f).applyTransformation(Mtx4::Scaling(0.01f, 8.0f, 0.01f)));

        // model sloupu z .obj - cesta funguje pri spusteni z korene repa
        // i z GLibpp/ (VS debugger ma working directory = ProjectDir)
        const char* objPath = "data/models/sloup.obj";
        if (!std::ifstream(objPath).good()) objPath = "../data/models/sloup.obj";

        // importer vybere loader podle pripony (dnes jen .obj)
        GLibpp::Assets::ModelImporter importer;
        auto columnMesh = resources.meshRegister(importer.load(objPath).mesh.flipFaces());

        // textura panelu - dekoduje ji Windows (GDI+), stejny fallback cesty jako model
        const char* texPath = "data/textures/tex.jpg";
        if (!std::ifstream(texPath).good()) texPath = "../data/textures/tex.jpg";
        scene.renderables.panelTexture = resources.textureRegister(GLibpp::Platform::ImageLoaderWin32::Load(texPath));

        // capture textury (zrcadlo framebufferu + vizualizace hloubky) -
        // velikost = klientska plocha okna, obsah plni Renderer::captureFrame
        // na konci kazdeho framu; placeholder barva je videt jen prvni frame.
        // Kanonicka TextureData je jen zakladatel residency - po resize okna
        // si Renderer dotahne residency target sam (targetResize)
        {
            const uint32_t w = window->getClientWidth();
            const uint32_t h = window->getClientHeight();

            GLibpp::Assets::TextureData fbTex;
            fbTex.width = w;
            fbTex.height = h;
            fbTex.pixels.assign(size_t(w) * h, 0xFF303030u);
            scene.renderables.fbTexture = resources.textureRegister(std::move(fbTex));

            GLibpp::Assets::TextureData depthTex;
            depthTex.width = w;
            depthTex.height = h;
            depthTex.pixels.assign(size_t(w) * h, 0xFF000000u);
            scene.renderables.depthTexture = resources.textureRegister(std::move(depthTex));
        }

        // zem: mrizka ma v mesh prostoru souradnice 0..size-1, takze translate
        // o -(size-1)/2 ji vycentruje na pocatek. Rozteč 1.0 (drive 0.5) dava
        // dvojnasobny rozsah v obou osach, tj. plocha [-29.5, 29.5]^2, PRI
        // NEZMENENEM poctu trojuhelniku - vlna je na Debug rozpocet citliva,
        // zdvojnasobeni hustoty (size 120) by ho probouralo
        Mtx4 gridWaveModel = Mtx4::Identity().rotateX(GLibpp::Math::deg2rad(90.0f)).translate(-29.5f, -29.5f, 0.0f);

        // panel stoji na zemi kousek od originu, celem k vychozi pozici kamery
        Mtx4 panelModel = Mtx4::Identity().translate(4.0f, 2.0f, 4.0f);

        // panely s produkty renderu (zrcadlo framebufferu + vizualizace hloubky)
        // jedou s autem: localTransform je offset v PROSTORU AUTA (+z = dopredu,
        // kamera jede za autem, takze XY quad na ni miri celem) a world matici
        // dodava buildDrawList per frame (carM, stejne jako kola); vedle sebe
        // nad strechou jako dva displeje. Sdileji quad s UV 0..1, obraz okna
        // je na ctverci mirne natazeny podle aspectu (dvirka: fit/letterbox =
        // UV z aspectu s presahem [0,1] v kratsim smeru + border-black shader).
        // Otoceni 180 st. kolem Y: quad ma u = 0 na -x, ale kamera se na panel
        // diva od -z, takze bez otoceni by byl obraz vodorovne preklopeny -
        // panely jsou displeje, ne zrcatka. Pozor na retezeni: transformace
        // se aplikuji v mesh prostoru zprava (scale -> rotace -> translate)
        Mtx4 fbPanelModel    = Mtx4::Identity().translate(-3.8f, 3.5f, 0.0f).rotateY(GLibpp::Math::deg2rad(180.0f)).scale(0.75f);
        Mtx4 depthPanelModel = Mtx4::Identity().translate(3.8f, 3.5f, 0.0f).rotateY(GLibpp::Math::deg2rad(180.0f)).scale(0.75f);

        scene.renderables.gridWave  = resources.meshInstanceRegister(waveMesh, gridWaveModel, Color::Grayscale(0.3f), true);
        scene.renderables.texPanel   = resources.meshInstanceRegister(panelMesh, panelModel, Color::Grayscale(0.55f), false);
        scene.renderables.fbPanel    = resources.meshInstanceRegister(panelMesh, fbPanelModel, Color::Grayscale(0.55f), false);
        scene.renderables.depthPanel = resources.meshInstanceRegister(panelMesh, depthPanelModel, Color::Grayscale(0.55f), false);

        // drateny ram zrcadla - sdili localTransform fbPanelu, takze sedi
        // presne kolem nej; wireframe cary ignoruji hloubku (debug overlay)
        scene.renderables.fbPanelFrame = resources.meshInstanceRegister(panelFrameMesh, fbPanelModel, Color::Grayscale(0.85f), true);
        scene.renderables.carBody   = resources.meshInstanceRegister(bodyMesh);
        scene.renderables.wheel     = resources.meshInstanceRegister(wheelMesh);
        scene.renderables.icosphere = resources.meshInstanceRegister(sphereMesh, Mtx4::Identity(), Color::Grayscale(0.7f), true);
        scene.renderables.icrBeam   = resources.meshInstanceRegister(icrMesh);

        // --- alej sloupu z .obj modelu, 10 instanci jedne geometrie ---
        // Sloup stoji zakladnou na y = 0 a rameno trci do -x, takze leva rada
        // se otoci o 180 st. a ramena obou rad miri nad vozovku. Pozice se
        // pecou do localTransform instance - je to staticka scenerie, world
        // matice pri kresleni zustava Identity (na rozdil od kol, kterym world
        // pocita logika kazdy tick).
        // Pozor na retezeni builderu: rotateY se aplikuje v mesh prostoru PRED
        // translate, takze sloup rotuje kolem sve osy, ne kolem originu sceny.
        // Odstup rady je zamerne maly: engine NEUMI clipping trojuhelniku
        // (trojuhelnik s vrcholem mimo frustum se cely zahodi), takze siroka
        // alej by u kamery mizela - pri fov 45 st. a kamere 12 j. za autem je
        // vodorovne videt jen ~+-6,4 j. Rada zacina za autem a mizi do dalky,
        // aby jich pri startu bylo v zaberu nekolik.
        {
            constexpr float laneX = 6.0f;    // odstup rady od osy vozovky
            constexpr float spacing = 12.0f; // rozestup sloupu podel jizdy (z)

            for (size_t i = 0; i < SceneRenderables::kColumnCount; ++i)
            {
                const bool leftSide = (i % 2) != 0;
                const float x = leftSide ? -laneX : laneX;
                const float z = -20.0f + float(i / 2) * spacing; // -20 .. +28

                Mtx4 columnModel = Mtx4::Identity().translate(x, 0.0f, z);
                if (leftSide) columnModel.rotateY(GLibpp::Math::deg2rad(180.0f));

                scene.renderables.columns[i] =
                    resources.meshInstanceRegister(columnMesh, columnModel, Color::Grayscale(0.55f), false);
            }
        }
    }

    void scenePublish(const LogicState& logicState, double lastLogicTick) {

        auto& writeBuffer = logicStateBuffered.get_write_buffer();
        writeBuffer = logicState;
        writeBuffer.tickInfo.lastLogicTick = lastLogicTick;
        logicStateBuffered.publish();
	}

    static void setupThreadPriority(int32_t priority)
    {
        // Nastavi prioritu JEN aktualniho vlakna.
        // To typicky zlepší stabilitu frameratu, aniž by to ničilo systém.
        HANDLE thread = GetCurrentThread();

        if (!SetThreadPriority(thread, priority)) {
            // Volitelné: lognout chybu, ale nepanikařit.
            // std::cerr << "Failed to set thread priority\n";
        }
    }

    void run()
    {

		// logic scheduler - bude volat updateLogic() s pevnou frekvenci, nezavisle na renderovani
        TimeManager timer(logicHz, true);
        TimeManager timer10Hz(10.0f);

        LogicState logicState;

        logicState.scene.camera = Camera::Demo(45);

        setupDemoResources(logicState.scene);

        running.start();

        // prvni publikovany stav musi nest platne handly - holy publish() by odeslal
        // default-konstruovany write buffer s INVALID handly
        scenePublish(logicState, 0.0);

		setupThreadPriority(THREAD_PRIORITY_HIGHEST);

        std::thread renderThread([this]() {
            setupThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);
            renderer->runLoop();
        });

        while (running.isRunning())
        {
            window->pollEvents();

            timer.tickAndDispatchAction([&](double dt, double lastLogicTick) {
                input.keyboard.update();
                updateLogic(static_cast<float>(dt), logicState.scene);
                scenePublish(logicState, lastLogicTick);
             });

            timer10Hz.tickAndDispatchAction([&](double dt) {



                /*
                // test jitteru App logiky
                static std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, 10);
                int random = dist(rng);
                lagTest(random);
                */
			});

            timer.waitUntilNextStep();
        }

        renderer->stop();

        if (renderThread.joinable())
        {
            renderThread.join();
        }
    }
};
