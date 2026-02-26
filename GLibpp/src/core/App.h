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

	App(int width = 800, int height = 600) {
	
		float fov = (float)width / (float)height;

		this->lookAt = Mtx4::lookAt(Vec4(0, 0, 6, 1), Vec4(0, 0, 0, 1), Vec4(0, 1, 0, 0));
		this->projection = Mtx4::perspective(45, fov, 0.01f, 20.0f);
		this->viewport = Mtx4(
			width/2.0f, 0.0f,  0.0f, width/2.0f,
			0.0f, height/2.0f, 0.0f, height/2.0f,
			0.0f, 0.0f,   1.0f, 0.0f,
			0.0f, 0.0f,   0.0f, 1.0f
		);
	}

	void init();
	void update(float dt);
	void render();

	void setWindow(WindowBuilder* window);


	void __work();
	void __cmdUpdate(float dt, float fps);

private:

	SceneState sceneState;
	Mtx4 lookAt;
	Mtx4 projection;
	Mtx4 viewport;

	


	WindowBuilder* win = nullptr;

};
