#pragma once
#include <memory>

#include "WindowWin32.h"
#include "ZeroAllocTripleBuffer.h"
#include "Scene.h"
using SceneBuffer = ZeroAllocTripleBuffer<Scene>;


#include "Renderer.h"
#include "RenderCommandList.h"
#include "Mtx4.h"
#include "Camera.h"
#include "RunState.h"
#include "RenderTargetDescriptor.h"
#include <atomic>
#include <thread>
#include "TimeManager.h"
#include "Input.h"
#include <random>

#define RENDER_BACKEND_DIB


#if defined(RENDER_BACKEND_DIB)
#include "DeviceDIB.h"
using RenderDevice = Render::DeviceDIB;
#elif defined(RENDER_BACKEND_STENCIL)
#include "DeviceStencil.h"
using RenderDevice = RenderDeviceStencil;
#else
#error "Neni definovan backend!"
#endif

class App {
private:

    using RendererEngine = Render::Renderer<RenderDevice>;

    std::unique_ptr<WindowWin32> window;

	//std::unique_ptr<RendererEngine> renderer; // obecny renderer, ktery pouziva RenderDevice (DIB, Stencil, ...), ktery se zvoli definici makra RENDER_BACKEND_XXX
    std::unique_ptr<Render::Renderer<Render::DeviceDIB>> renderer; // pro vyvoj pouzijeme takhle explicitne, kvuli napovidani v IDE...

    Scene scene;

    bool fullscreen = false;
    RunState running;

    GLibpp::Input input;

    float logicHz = 30;

    bool checkWindowInitialized() const {
        if (window.get() == nullptr) {
            throw std::runtime_error("Window is not initialized");
        }
        return true;
	}


	SceneBuffer sceneBuffered;

public:

    App() = default;
	~App() = default;

	void setFullscreenMode(bool fullscreen) { 
		checkWindowInitialized();
		window->setFullscreenMode(fullscreen);
    }

    void initialize(uint32_t width, uint32_t height, const std::wstring& preferedDisplayName = L"")
    {
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
            renderer = std::make_unique<RendererEngine>(*window, sceneBuffered, logicHz);
        }

        {
            scene.camera = Camera::Demo(45);
            scene.modelMatrix = Mtx4::Identity();

            //renderer->updateScene(scene);
            //renderer->updateScene(scene);
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

    void processInputs()
    {
    }

    void updateLogic(double dtExtra)
    {
		float dt = std::clamp(static_cast<float>(dtExtra), 0.0f, 1.0f);

		//std::cout << "Updating logic with dt = " << dt << " seconds" << std::endl;

        //scene.camera.position.x += 10.0f * static_cast<float>(dt);
        //scene.camera.position.y += 10*sinf(0.1f * static_cast<float>(dt));



        
        if (input.keyboard.isDown(KeyMap::KEY_ENTER)) {
			scene.modelMatrix.rotateY(1.0f * dt);
        }

        if (input.keyboard.isDown(KeyMap::KEY_UP)) {
            scene.camera.move(Vec4(0.0f, 0.0f, -1.0f, 0.0f) * dt);
        }

        if (input.keyboard.isDown(KeyMap::KEY_DOWN)) {
            scene.camera.move(Vec4(0.0f, 0.0f, 1.0f, 0.0f) * dt);
        }

        if (input.keyboard.isDown(KeyMap::KEY_LEFT)) {
            scene.camera.rotate(-1.0f * dt, 0.0f);
        }

        if (input.keyboard.isDown(KeyMap::KEY_RIGHT)) {
            scene.camera.rotate(1.0f * dt, 0.0f);
        }
        

		scene.modelMatrix.rotateY(scene.rotationSpeed * dt); 
		scene.camera.position.z += scene.cameraSpeed * dt;


        scene.test += 1.0f;
        //std::cout << "test inc(1): " << scene.test << std::endl;

    }

    void run()
    {
        {
            auto& rm = renderer->getResourceManager();
            auto texHandle = rm.targetCreate(Render::RenderTargetDescriptor::FramebufferRGBA32bit(window->getClientWidth(), window->getClientHeight()));
            auto meshInstHandle = rm.meshRegister(MeshInstance());
        }


		// logic scheduler - bude volat updateLogic() s pevnou frekvenci, nezavisle na renderovani
        TimeManager timer(logicHz);
        TimeManager timer10Hz(10.0f);

        //TimeManager timerOneSecond(1.0); // pro výpočet FPS každou sekundu

        running.start();


        sceneBuffered.publish();
        sceneBuffered.publish();

        std::thread renderThread([this]() {
            //renderer->updateScene(scene); // to tu asi ani byt nemusi
            renderer->runLoop();
            });

        while (running.isRunning())
        {
            window->pollEvents();

            timer.tickAndDispatchAction([&](double dt) {
                input.keyboard.update();
                updateLogic(dt);
                //renderer->updateScene(scene);



                renderer->updateLastLogicTick(timer.sinceStart());

                auto& writeBuffer = sceneBuffered.get_write_buffer();
                writeBuffer = scene;
                sceneBuffered.publish();
                //std::cout << "Producer: published" << std::endl;

                //std::cout << sceneBuffered.toString() << std::endl;
             });

            timer10Hz.tickAndDispatchAction([&](double dt) {
                // zatim nic

                /*
                static std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> dist(0, 10);
                int random = dist(rng);
                lagTest(random);
                */
			});


            /*
            if (false) {
                timerOneSecond.tickAndDispatchAction([&](double dt) {
                    double fps = timer.getFps();
                    double low1 = timer.getLow1Percent();
                    double low01 = timer.getLowPoint1Percent();
                    window->setTitle(std::format(
                        "FPS: {:.2f}, 1% Low: {:.2f}, 0.1% Low: {:.2f}",
                        fps, low1, low01
                    ));
                    });
            }
            */

            //timer.waitUntilNextStep(); // to ted delat nebudeme
        }

        renderer->stop();

        if (renderThread.joinable())
        {
            renderThread.join();
        }
    }
};



    /*
    void render()
    {
        // create render target using descriptor helper
        
        renderer->runLoop(
            RenderCommandList::Create()
            .clear({ 232,232,232,255 })
            .setViewport(0, 0, window->getClientWidth(), window->getClientHeight())
            .setMatrixProjection(Mtx4::Perspective(
                45.0f,
                window->getClientWidth() / (float)window->getClientHeight(),
                0.01f,
                1000.0f
            ))
            .setMatrixView(Mtx4::LookAt(
                Vec4(1.0f, 1.0f, 1.0f, 0.0f),
                Vec4(0.0f, 0.0f, 0.0f, 0.0f),
                Vec4(0.0f, 1.0f, 0.0f, 1.0f)
            ))
            .bindMesh(MeshHandle{ 1 }, MaterialHandle{ 1 })
            .draw()
            .bindMesh(MeshHandle{ 1 }, MaterialHandle{ 1 })
            .draw()
            .setMatrixView(Mtx4::Identity())
            .bindMesh(MeshHandle{ 2 }, MaterialHandle{ 2 })
            .draw()
        );
    }*/


