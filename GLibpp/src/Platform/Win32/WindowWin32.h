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
#include "Win32DC.h"

class WindowWin32;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class WindowWin32 
{

	friend LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM); // pouziva clenske metody tridy WindowWin32 -> proto friend

private:
	std::function<void(KeyMap, bool)> onKeyEvent;
	std::function<void()> onClose;
	std::function<void(uint32_t width, uint32_t height)> onResize;

	HWND hwnd = nullptr;
	int width;
	int height;
	bool maximized = false;

public:

	WindowWin32(int width, int height, bool maximized) :
		hwnd(nullptr),
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

	WindowWin32(const WindowWin32&) = delete;
	WindowWin32& operator=(const WindowWin32&) = delete;

	static const wchar_t* GetClassName() {
		return L"EngineWindowClass";
	}
	
	void setTitle(const std::string& title);

	void setFullscreenMode(bool fullscreen)
	{
		if (fullscreen) {
			removeOverlapProperty();
			resizeWindowToFillScreen();
			hideCursor();
		}
		else {
			// @todo: restore previous window size and position
			showCursor();
		}
	}

	bool build();
	
	void waitEvents() const noexcept;
	void pollEvents() const noexcept;

	void setKeyCallback(std::function<void(KeyMap, bool)> cb) noexcept;
	void setOnCloseCallback(std::function<void()> cb) noexcept;
	void setOnResizeCallback(std::function<void(uint32_t, uint32_t)> cb) noexcept;

	void removeOverlapProperty();
	void resizeWindowToFillScreen();
	void glibRegisterRawInputDevices();

	HWND getHwnd() const noexcept;
	uint32_t getWidth() const noexcept { return static_cast<uint32_t>(width); }
	uint32_t getHeight() const noexcept { return static_cast<uint32_t>(height); }
	uint32_t getClientWidth() const noexcept;
	uint32_t getClientHeight() const noexcept;
	void hideCursor() const noexcept;
	void showCursor() const noexcept;

	HDC getDeviceContext() const {
		return GetDC(hwnd);
	}
	void releaseDeviceContext(HDC hdc) const {
		ReleaseDC(hwnd, hdc);
	}

private:

	bool RegisterWindowClass(HINSTANCE hInstance);
	LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void setHwnd(HWND hwnd) { this->hwnd = hwnd; }
};