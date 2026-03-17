#include "EngineApp.h"

#include <thread>
#include "core/render/Renderer.h"

bool EngineApp::init()
{
	window = std::make_unique<WindowWin32>(1366, 768, false);

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

	while (running) {

		window->pollEvents();
		if (keyboard[KeyMap::KEY_ESC]) 
		{
			renderer->stop();
			running = false;
		}
	}

	threadRender.join();

	return true;
}
