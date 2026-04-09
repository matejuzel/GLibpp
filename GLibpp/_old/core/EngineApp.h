#pragma once

#include "window/WindowWin32.h"
#include "render/Renderer.h"
#include "render/IRenderDevice.h"
#include "render/backendDIB/RenderDeviceDIB.h"
#include "core/input/Keyboard.h"
#include "window/ConsoleDoubleBuffer.h"

class EngineApp 
{
private:
	std::unique_ptr<WindowWin32> window = nullptr;
	std::unique_ptr<IRenderDevice> device = nullptr;
	std::unique_ptr<Renderer> renderer = nullptr;
	
	uint32_t width = 80;
	uint32_t height = 60;

	bool fullscreen = false;
	bool running = false;

	Keyboard keyboard;
	ConsoleDoubleBuffer console;

public:

	EngineApp() = default;
	~EngineApp() = default;

	bool init() {

		try {

			// WINDOW CREATION AND INIT
			{
				window = std::make_unique<WindowWin32>(width, height, false);

				if (!window->build()) throw new std::runtime_error("Chyba pri vytvareni okna");

				if (fullscreen)
				{
					window->removeOverlapProperty();
					window->resizeWindowToFillScreen();
					window->hideCursor();
				}

				window->setOnCloseCallback([this]() {
					running = false;
					});

				window->setKeyCallback([this](KeyMap key, bool pressed) {
					onKeyCallback(key, pressed);
					});

				window->glibRegisterRawInputDevices();
			}

			// RENDER DEVICE CREATION
			{
				device = std::make_unique<RenderDeviceDIB>(window.get(), window->getClientWidth(), window->getClientHeight(), 32);
			}

			// RENDERER CREATION
			{
				renderer = std::make_unique<Renderer>(device.get(), window->getClientWidth(), window->getClientHeight());
			}
			

		}
		catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
			return false;
		}

		return true;
	}

	bool runLoop();
	void updateLogic(double dt) noexcept;
	void updateConsole(double dt) noexcept;

	void onKeyCallback(KeyMap key, bool pressed) noexcept;
};