#pragma once

#include "window/WindowProcedure.h"
#include "window/WindowWin32.h"

#include <windows.h>
#include <iostream>

class EngineApp 
{
private:
	std::unique_ptr<WindowWin32> window;


public:

	EngineApp() = default;
	~EngineApp() = default;

	bool init()
	{
		window = std::make_unique<WindowWin32>(800, 600, WindowProc, false);

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
		bool running = true;

		while (GetMessage(&msg, nullptr, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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