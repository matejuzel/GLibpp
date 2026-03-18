#pragma once

#include "window/WindowWin32.h"
#include "core/render/Renderer.h"
#include "core/input/Keyboard.h"
#include "window/ConsoleDoubleBuffer.h"

class EngineApp 
{
private:
	std::unique_ptr<WindowWin32> window = nullptr;
	std::unique_ptr<Renderer> renderer = nullptr;
	
	uint32_t width = 200;
	uint32_t height = 160;

	bool running = false;

	Keyboard keyboard;
	ConsoleDoubleBuffer console;

public:

	EngineApp() = default;
	~EngineApp() = default;

	bool init();
	bool runLoop();
	void updateLogic(double dt);
	void updateConsole(double dt);

};