#pragma once
#include <memory>

#include "WindowWin32.h"
#include "Renderer.h"
#include "RenderCommandList.h"
#include "Mtx4.h"
#include "Camera.h"
#include <atomic>
#include <thread>

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

    bool fullscreen = false;
    std::atomic<bool> running = { true };

    Camera camera = Camera::Demo(45);


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

    void initialize(uint32_t width, uint32_t height) 
    {
        {
            // WINDOW
            window = std::make_unique<WindowWin32>(width, height, false);

            if (!window->build()) {
                throw std::runtime_error("Failed to create window");
            }

            if (fullscreen)
            {
                window->removeOverlapProperty();
                window->resizeWindowToFillScreen();
                window->hideCursor();
            }

            window->setOnCloseCallback([this]() {
                stop();
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
            renderer = std::make_unique<RendererEngine>(*window);
        }
	}

    void onKeyCallback(KeyMap key, bool pressed) 
    {
        if (key == KeyMap::KEY_ESC && pressed) {
            stop();
        }

		std::cout << "Key event: " << (pressed ? "Pressed" : "Released") << " " << static_cast<int>(key) << std::endl;
	}

    void run()
    {
		running.store(true, std::memory_order_relaxed);
        
        std::thread renderThread([this]() {
            renderer->runLoop(camera);
        });

        while (isRunning())
        {
            window->pollEvents();
        }

        renderer.get()->stop();

        if (renderThread.joinable())
        {
            renderThread.join();
        }
	}

    void stop()
    {
        running.store(false, std::memory_order_relaxed);
    }

    bool isRunning() const
    {
        return running.load(std::memory_order_relaxed);
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