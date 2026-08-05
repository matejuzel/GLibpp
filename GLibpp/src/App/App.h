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
using RenderDevice = Render::DeviceDIB;
#else
#error "Neni definovan backend!"
#endif

/*
Implementovat nekdy v budoucnu:
	Origin Rebasing
        pri rozsahlem svete a velkym offsetu kamery se zacne projevovat snizujicici se presnost floatu.
        Resenim je Origin Rebasing, ktery pravidelne posouva svet tak aby kamera byla co nejblize k originu.
*/

class App {
private:

    using RendererEngine = Render::Renderer<RenderDevice>;

    std::unique_ptr<WindowWin32> window;

    // kanonicke uloziste assetu (meshe, instance) - obsah vlastni App, renderer dostava referenci
    // deklarovano PRED rendererem: renderer drzi referenci, musi tedy zaniknout driv
    Render::ResourceManager resources;

	//std::unique_ptr<RendererEngine> renderer; // obecny renderer, ktery pouziva RenderDevice (DIB, Stencil, ...), ktery se zvoli definici makra RENDER_BACKEND_XXX
    std::unique_ptr<Render::Renderer<Render::DeviceDIB>> renderer; // pro vyvoj pouzijeme takhle explicitne, kvuli napovidani v IDE...

    bool fullscreen = false;
    RunState running;

    GLibpp::Input input;

    float logicHz = 60;

    bool checkWindowInitialized() const {
        if (window.get() == nullptr) {
            throw std::runtime_error("Window is not initialized");
        }
        return true;
	}

    LogicStateBuffered logicStateBuffered;

public:

    App() = default;
	~App() = default;

	void setFullscreenMode(bool fullscreen) {
		checkWindowInitialized();
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

            std::cout << Quaternion::Slerp(q1, q2, 0.5f).toMatrix() << std::endl;

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

            window->glibRegisterRawInputDevices();
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

        if (flag) scene.car.speedDown(0.01f);


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
        constexpr float stiffness      = 1.0f;  // rychlost dotahovani pozice [1/s]

        Vec4 carPos = scene.car.model.getPosition();
        Vec4 forward = scene.car.model.getHeading() * Vec4(0.0f, 0.0f, 1.0f, 0.0f);

        Vec4 desired = carPos - forward * followDistance + Vec4(0.0f, followHeight, 0.0f, 0.0f);

        float alpha = 1.0f - std::exp(-stiffness * dt);
        Vec4 newPos = Vec4::Lerp(scene.camera.position, desired, alpha);

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
        auto icrMesh = resources.meshRegister(MeshFactory::CreateCube(0.1f).applyTransformation(Mtx4::Scaling(0.01f, 8.0f, 0.01f)));

        Mtx4 gridWaveModel = Mtx4::Identity().rotateX(GLibpp::Math::deg2rad(90.0f)).translate(-25.0f, -25.0f, 0.0f).scale(0.5f);

        scene.renderables.gridWave  = resources.meshInstanceRegister(waveMesh, gridWaveModel, Color::Grayscale(0.3f), true);
        scene.renderables.carBody   = resources.meshInstanceRegister(bodyMesh);
        scene.renderables.wheel     = resources.meshInstanceRegister(wheelMesh);
        scene.renderables.icosphere = resources.meshInstanceRegister(sphereMesh, Mtx4::Identity(), Color::Grayscale(0.7f), true);
        scene.renderables.icrBeam   = resources.meshInstanceRegister(icrMesh);
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
