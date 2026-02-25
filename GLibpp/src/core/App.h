#pragma once

#include "core/input/Keyboard.h"
#include "math/Mtx4.h"
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowBuilder.h"
#include "core/SceneState.h"
#include <windows.h>

class App
{
public:

	static App& instance() {
		static App inst;
		return inst;
	}

	Mtx4 mtx;
	Keyboard keyboard;

	ConsoleDoubleBuffer console;

	App() = default;

	void init();
	void update(float dt);
	void render();

	void setWindow(WindowBuilder* window);


	void __work();
	void __cmdUpdate(float dt);

private:

	SceneState sceneState;
	WindowBuilder* win = nullptr;

};
