#pragma once

#include "core/input/Keyboard.h"
#include "math/Mtx4.h"

class App
{
public:

	static App& instance() {
		static App inst;
		return inst;
	}

	Mtx4 mtx;
	Keyboard keyboard;

	App() = default;

	void init();
	void update();
	void render();


	void __work();

private:


};
