#pragma once
#include <windows.h>
#include <string>

class WindowBuilder {

public:

	using WindowCallback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
	using MainLoopCallback = bool(*)(float logicHz);

	WindowBuilder(MainLoopCallback mainLoopProc, WindowCallback proc);

	bool init();
	bool run(float logicHz);
	//bool mainLoop();

	static void consoleSetFixedViewport();
	static void consolePrint(std::string text);
	static void consoleClear();

private:
	
	HWND hwnd;
	HINSTANCE hInstance;
	MainLoopCallback mainLoopProc;
	WindowCallback callback = nullptr;

	void glibRegisterRawInputDevices();

};