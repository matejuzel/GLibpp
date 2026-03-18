#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <iostream>
#include <type_traits>
#include <functional>
#include <memory>
#include "math/Vec4.h"
#include "core/input/Keymap.h"

class WindowWin32;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class WindowWin32 
{

	friend LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM); // pouziva clenske metody tridy WindowWin32 -> proto friend

private:
	std::function<void(KeyMap, bool)> onKeyEvent;
	std::function<void()> onClose;

	HWND hwnd = nullptr;
	//HINSTANCE hInstance = nullptr;
	int width;
	int height;
	bool maximized = false;

public:

	WindowWin32(int width, int height, bool maximized) :
		hwnd(nullptr),
		//hInstance(nullptr),
		width(width),
		height(height),
		maximized(maximized)
	{
	}

	~WindowWin32()
	{
		if (hwnd) {
			DestroyWindow(hwnd);
			hwnd = nullptr;
		}
	}

	static const wchar_t* GetClassName() {
		return L"EngineWindowClass";
	}
	
	bool build();
	
	void waitEvents() const noexcept;
	void pollEvents() const noexcept;

	void setKeyCallback(std::function<void(KeyMap, bool)> cb) noexcept;
	void setOnCloseCallback(std::function<void()> cb) noexcept;

	void removeOverlapProperty();
	void resizeWindowToFillScreen();
	void glibRegisterRawInputDevices();

	HWND getHwnd() const;
	uint32_t getWidth() const { return static_cast<uint32_t>(width); }
	uint32_t getHeight() const { return static_cast<uint32_t>(height); }

private:

	bool RegisterWindowClass(HINSTANCE hInstance);
	LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void setHwnd(HWND hwnd) { this->hwnd = hwnd; }
};