#pragma once
#include <windows.h>

class WindowBuilder {

public:

	using WindowCallback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
	using MainLoopCallback = bool(*)();

	WindowBuilder(MainLoopCallback mainLoopProc, WindowCallback proc);

	bool init();
	bool create();
	//bool mainLoop();

private:
	
	HWND hwnd;
	HINSTANCE hInstance;
	MainLoopCallback mainLoopProc;
	WindowCallback callback = nullptr;

	void glibRegisterRawInputDevices();

};