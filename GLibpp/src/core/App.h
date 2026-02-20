#pragma once

#include "core/input/KeyCode.h"
#include "math/Mtx4.h"

class App
{
public:

	static App& instance() {
		static App inst;
		return inst;
	}

	Mtx4 mtx;

	App() = default;

	void work();
	void onKeydown(KeyCode keyCode);

private:


};
