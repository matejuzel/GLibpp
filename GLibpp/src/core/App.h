#pragma once

#include "core/input/Keyboard.h"
#include "math/Mtx4.h"
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowBuilder.h"
#include "core/SceneState.h"


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
	void update(float dt);
	void render();

	void setWindow(WindowBuilder* window);


	void __work();
	void __cmdUpdate(float dt, float fps);

private:

	SceneState sceneState;
    // move projection and viewport to scene state
    // now SceneState contains per-scene lookAt/projection/viewport

	


	WindowBuilder* win = nullptr;

};
