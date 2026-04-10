#pragma once
#include <memory>

#include "IRenderDevice.h"
#include "WindowWin32.h"
#include "Renderer.h"
#include "RenderCommandList.h"
#include "RenderDeviceDIB.h"
#include "Mtx4.h"

class App {
private:
    WindowWin32 window;
    std::unique_ptr<IRenderDevice> device;
    std::unique_ptr<Renderer> renderer;

    bool fullscreen = false;
	bool running = true;

public:
    App() : window(800, 600, false) {}

    void initialize() 
    {
        {
            // WINDOW
            if (!window.build()) {
                throw std::runtime_error("Failed to create window");
            }

            if (fullscreen)
            {
                window.removeOverlapProperty();
                window.resizeWindowToFillScreen();
                window.hideCursor();
            }

            window.setOnCloseCallback([this]() {
                running = false;
                });

            window.setKeyCallback([this](KeyMap key, bool pressed) {
                onKeyCallback(key, pressed);
                });

            window.glibRegisterRawInputDevices();
        }
        
        {
            // DEVICE
            device = std::make_unique<RenderDeviceDIB>(window.getHwnd());
        }

        {
            // RENDERER
            renderer = std::make_unique<Renderer>(*device, window.getClientWidth(), window.getClientHeight());
        }
	}

    void onKeyCallback(KeyMap key, bool pressed) 
    {
        if (key == KeyMap::KEY_ESC && pressed) {
            running = false;
        }

		std::cout << "Key event: " << (pressed ? "Pressed" : "Released") << " " << static_cast<int>(key) << std::endl;
	}

    void run()
    {
        while (running) {
            window.pollEvents();
            render();
        }
	}

    void render()
    {
        // create render target using descriptor helper

        renderer->runLoop(
            RenderCommandList::Create()
            .clear({ 0,0,0,255 })
            .setViewport(0, 0, window.getClientWidth(), window.getClientHeight())
            .setMatrixProjection(Mtx4::Perspective(
                45.0f,
                window.getClientWidth() / (float)window.getClientHeight(),
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
    }
};