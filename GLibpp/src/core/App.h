#pragma once

#include "core/input/Keyboard.h"
#include "math/Mtx4.h"
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowBuilder.h"
#include "core/SceneState.h"
#include "core/GameLoop.h"


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

	void init(int width, int height);
	bool runGameLoop();
	void update(float dt);
	void render();

	void setWindow(WindowBuilder* window);


	void __work();
	void __cmdUpdate(float dt, float fps);

private:

	WindowBuilder* win = nullptr;
	GameLoop gameLoop;
	SceneState sceneState;
	
	bool antialiasing = true;
};
