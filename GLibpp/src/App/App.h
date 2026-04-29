#pragma once
#include <memory>

#include "WindowWin32.h"
#include "Renderer.h"
#include "RenderCommandList.h"
#include "Mtx4.h"
#include "Camera.h"
#include "RunState.h"
#include "RenderTargetDescriptor.h"
#include <atomic>
#include <thread>
#include "TimeManager.h"

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

    float logicHz = 30.0f;

    bool checkWindowInitialized() const {
        if (window.get() == nullptr) {
            throw std::runtime_error("Window is not initialized");
        }
        return true;
	}

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
            renderer = std::make_unique<RendererEngine>(*window, logicHz);
        }

        {
            scene.camera = Camera::Demo(45);
            scene.modelMatrix = Mtx4::Identity();

            renderer->updateScene(scene);
            renderer->updateScene(scene);
        }

	}

    void onKeyCallback(KeyMap key, bool pressed) 
    {
        if (key == KeyMap::KEY_ESC && pressed) {
            running.stop();
        }

		if (key == KeyMap::KEY_ENTER) scene.rotationSpeed += 0.1f;
		if (key == KeyMap::KEY_UP) scene.cameraSpeed -= 0.1f;
        if (key == KeyMap::KEY_DOWN) scene.cameraSpeed += 0.1f;
        if (key == KeyMap::KEY_LEFT) scene.cameraRotationSpeed -= 0.1f;
        if (key == KeyMap::KEY_RIGHT) scene.cameraRotationSpeed += 0.1f;
		std::cout << "Key event: " << (pressed ? "Pressed" : "Released") << " " << static_cast<int>(key) << std::endl;
	}

    void updateLogic(double dt)
    {
		//std::cout << "Updating logic with dt = " << dt << " seconds" << std::endl;

        //scene.camera.position.x += 10.0f * static_cast<float>(dt);
        //scene.camera.position.y += 10*sinf(0.1f * static_cast<float>(dt));


		scene.modelMatrix.rotateY(scene.rotationSpeed * static_cast<float>(dt)); 
		scene.camera.position.z += scene.cameraSpeed * static_cast<float>(dt);

		renderer->updateScene(scene);



    }

    void run()
    {
        {
            auto& rm = renderer->getResourceManager();
            auto texHandle = rm.targetCreate(Render::RenderTargetDescriptor::FramebufferRGBA32bit(window->getClientWidth(), window->getClientHeight()));
            auto meshInstHandle = rm.meshRegister(MeshInstance());
        }

        

        TimeManager timer(logicHz);

        running.start();
        
        std::thread renderThread([this]() {

			renderer->updateScene(scene);

            renderer->runLoop();
        });

        while (running.isRunning())
        {
            window->pollEvents();

            timer.tickAndDispatchAction([&](double dt) {
                updateLogic(dt);
				//std::cout << "FPS: " << timer.getFps() << std::endl;
			});

            // Výpočet: kolik sekund zbývá, než akumulátor dosáhne m_fixedDelta?
            //double timeToWait = timer.getFixedDelta() - timer.getAccumulator();

            /*
            if (timeToWait > 0.001) { // Spíme jen, pokud je na to čas (víc než 1ms)
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(timeToWait * 1000)));
            }
            */

            //timer.waitUntilNextStep();

        }

        renderer->stop();

        if (renderThread.joinable())
        {
            renderThread.join();
        }

	}



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
};