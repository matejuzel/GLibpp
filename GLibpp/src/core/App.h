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
	void update(float dt);
	void render();



	void __work();
	void __cmdUpdate(float dt);

private:

	double cmdAccumulator = 0.0;
	const double cmdInterval = 1.0 / 30.0;

};
