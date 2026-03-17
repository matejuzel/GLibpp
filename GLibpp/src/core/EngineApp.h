#pragma once

#include "window/WindowWin32.h"

#include "core/input/Keymap.h"
#include "core/input/Keyboard.h"
#include "core/render/Renderer.h"

#include <windows.h>
#include <iostream>
#include <thread>

class EngineApp 
{
private:
	std::unique_ptr<WindowWin32> window = nullptr;
	std::unique_ptr<Renderer> renderer = nullptr;
	Keyboard keyboard;
	bool running;

public:

	EngineApp() = default;
	~EngineApp() = default;

	bool init();
	bool runLoop();

private:

	bool buildWindow();

};