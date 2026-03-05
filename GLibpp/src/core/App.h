#pragma once

#include "core/input/Keyboard.h"
#include "math/Mtx4.h"
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowBuilder.h"
#include "core/SceneState.h"
#include "core/GameLoop.h"
#include "core/render/Renderer.h"
#include <utils/timer/Fps.h>


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
	WindowBuilder* getWindowPtr() const { return this->win; }


	void __work();
	void __cmdUpdate(float dt);

	SceneState& getSceneState() { return this->sceneState; }

	void setRendererPtr(Renderer* renderer) {
		this->renderer = renderer;
	}

	Fps& getFps() { return this->fps; }

private:

	WindowBuilder* win = nullptr;
	GameLoop gameLoop;
	SceneState sceneState;
	bool antialiasing = true;

	Renderer* renderer = nullptr;

	Fps fps;
};
