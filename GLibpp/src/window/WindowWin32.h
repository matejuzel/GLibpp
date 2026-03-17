#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <iostream>
#include "math/Vec4.h"

class WindowWin32 {

private:

	using WindowCallback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
	HWND hwnd = nullptr;
	HINSTANCE hInstance = nullptr;
	WindowCallback callback = nullptr;
	int width;
	int height;
	bool maximized = false;

public:

	WindowWin32(int width, int height, WindowCallback proc, bool maximized) : 
		hwnd(nullptr),
		hInstance(nullptr),
		callback(proc),
		width(width),
		height(height),
		maximized(maximized)
	{}

	static const wchar_t* GetClassName() {
		return L"EngineWindowClass";
	}

	bool RegisterWindowClass(HINSTANCE hInstance, WindowCallback callback);

	bool build();
	HWND getHwnd() const;

	static void consoleSetFixedViewport();
	static void consolePrint(std::string text);
	static void consoleClear();

	void removeOverlapProperty();
	void resizeWindowToFillScreen();
	void glibRegisterRawInputDevices();

	uint32_t getWidth() const { return static_cast<uint32_t>(width); }
	uint32_t getHeight() const { return static_cast<uint32_t>(height); }
	
};