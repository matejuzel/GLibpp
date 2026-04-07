#pragma once

#include "core/input/Keyboard.h"
#include "math/Mtx4.h"
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowWin32.h"
#include "scene/SceneState.h"
#include "core/GameLoop.h"
#include "render/Renderer.h"
#include <utils/timer/Fps.h>
#include "core/MeshInstanceRegistry.h"


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

	//App() = default;

	void init();
	bool runGameLoop();
	void update(float dt);
	
	void __cmdUpdate(float dt);


	Fps& getFps() { return this->fps; }


	Renderer* renderer;

private:

	SceneState sceneState;
	AssetRegistry<Material> materialRegistry;
	AssetRegistry<Mesh> meshRegistry;

	//MeshInstanceRegistry meshInstanceRegistry;



	GameLoop gameLoop;
	bool antialiasing = true;

	//Renderer* renderer = nullptr;

	Fps fps;
	Fps fpsLogic;
};
