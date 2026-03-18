#include "EngineApp.h"

#include <thread>
#include "utils/timer/FixedTimestep.h"
#include "utils/timer/HighResTimer.h"

bool EngineApp::init()
{
	window = std::make_unique<WindowWin32>(width, height, false);

	window->setOnCloseCallback(
		[this]() {
			this->running = false;
		}
	);

	window->setKeyCallback(
		[this](KeyMap key, bool pressed) {
			if (pressed)
			{
				this->keyboard.keyDown(key);
				std::cout << "down" << std::endl;
			}
			else
			{
				this->keyboard.keyUp(key);
			}
		}
	);

	if (!window->build())
	{
		std::cerr << "Chyba pri vytvareni okna" << std::endl;
		return false;
	}

	renderer = std::make_unique<Renderer>(window.get());
	renderer->init(window->getWidth(), window->getHeight());

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

void EngineApp::updateLogic(double dt)
{
	// @todo
}

void EngineApp::updateConsole(double dt)
{
	console.clearBack();

	console.write(2, std::to_string(dt));

	console.present();
}
