#include "EngineApp.h"

#include <thread>
#include "utils/timer/FixedTimestep.h"
#include "utils/timer/HighResTimer.h"

bool EngineApp::init()
{
	
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
		

		// RENDERER CREATION
		renderer = std::make_unique<Renderer>(window.get(), window->getClientWidth(), window->getClientHeight());

	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return false;
	}
	
	return true;
}

bool EngineApp::runLoop()
{
	std::thread threadRender = std::thread(&Renderer::runRenderLoop, renderer.get());

	MSG msg = {};
	running = true;

	HighResTimer timer;
	FixedTimestep timestepLogic(30.0f);
	FixedTimestep timestepConsole(30.0f);

	while (running) {

		double frameTime = timer.tick();
		if (frameTime > 0.25) frameTime = 0.25;

		{	// POLL EVENTS
			window->pollEvents();
			if (keyboard[KeyMap::KEY_ESC])
			{
				renderer->stop();
				running = false;
			}
		}
		
		{	// UPDATE LOGIC
			for (int i = 0; i < timestepLogic.consume(frameTime); i++)
			{
				updateLogic(timestepLogic.getDt());
			}
		}

		{	// CONSOLE LOGIC
			for (int i = 0; i < timestepConsole.consume(frameTime); i++)
			{
				updateConsole(timestepConsole.getDt());
			}
		}
		
	}

	threadRender.join();

	return true;
}

void EngineApp::updateLogic(double dt) noexcept
{

	

	// @todo
}

void EngineApp::updateConsole(double dt) noexcept
{
	console.clearBack();
	console.write(2, "updateConsole dt: " + std::to_string(dt));
	console.write(3, "Pressed key: " + keyboard.toString());
	console.present();
}

void EngineApp::onKeyCallback(KeyMap key, bool pressed) noexcept
{
	if (pressed)
	{
		this->keyboard.keyDown(key);
	}
	else
	{
		this->keyboard.keyUp(key);
	}
}
