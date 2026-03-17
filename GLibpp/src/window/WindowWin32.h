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



class WindowWin32 {

private:

	//using WindowCallback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);

private:
	std::function<void(KeyMap, bool)> onKeyEvent;
	std::function<void()> onClose;

	HWND hwnd = nullptr;
	HINSTANCE hInstance = nullptr;
	//WindowCallback callback = nullptr;
	int width;
	int height;
	bool maximized = false;

public:

	void setKeyCallback(std::function<void(KeyMap, bool)> cb)
	{
		onKeyEvent = std::move(cb);
	}

	void setOnCloseCallback(std::function<void()> cb)
	{
		onClose = std::move(cb);
	}

	void waitEvents() const noexcept
	{
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void pollEvents() const noexcept
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WindowWin32(int width, int height, bool maximized) : 
		hwnd(nullptr),
		hInstance(nullptr),
		width(width),
		height(height),
		maximized(maximized)
	{}

	~WindowWin32() 
	{
		if (hwnd) {
			DestroyWindow(hwnd);
			hwnd = nullptr;
		}
	}

	LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{

		case WM_KEYDOWN:
			if (onKeyEvent) onKeyEvent(KEYMAP[wParam], true);
			return 0;

		case WM_KEYUP:
			if (onKeyEvent) onKeyEvent(KEYMAP[wParam], false);
			return 0;

		case WM_INPUT:
			// veskerou input logiku zpracujeme nahore, takze zbytek tady ignorujeme
			return 0;

		case WM_CLOSE:
			if (onClose) onClose();
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	static const wchar_t* GetClassName() {
		return L"EngineWindowClass";
	}

	bool RegisterWindowClass(HINSTANCE hInstance);

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

	void setHwnd(HWND hwnd) { this->hwnd = hwnd; }
	
};