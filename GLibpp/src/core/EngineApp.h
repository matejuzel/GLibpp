#pragma once

#include "window/WindowWin32.h"

#include "core/input/Keymap.h"
#include "core/input/Keyboard.h"

#include <windows.h>
#include <iostream>

class EngineApp 
{
private:
	std::unique_ptr<WindowWin32> window;
	Keyboard keyboard;
	bool running;


public:

	EngineApp() = default;
	~EngineApp() = default;

	bool init()
	{
		window = std::make_unique<WindowWin32>(800, 600, false);

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
				}
				else
				{
					this->keyboard.keyUp(key);
				}
			}
		);

		if (!buildWindow())
		{
			std::cerr << "Chyba pri vytvareni okna" << std::endl;
			return false;
		}

		return true;
	}


	bool runLoop()
	{
		MSG msg = {};
		running = true;
		
		while (running) {
		
			window->pollEvents();
			if (keyboard[KeyMap::KEY_ESC]) running = false;

		}

		return true;
	}

private:

	bool buildWindow()
	{
		if (!window->build()) return false;

		return true;
	}

};