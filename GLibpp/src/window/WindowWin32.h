#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include "math/Vec4.h"

class WindowWin32 {

public:

	using WindowCallback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
	//using MainLoopCallback = bool(*)(float logicHz);

	WindowWin32(int width, int height, WindowCallback proc);

	bool build();
	HWND getHwnd() const;

	static void consoleSetFixedViewport();
	static void consolePrint(std::string text);
	static void consoleClear();

	//bool DIB_init();

	void DIB_removeOverlapProperty() {
		// odebere oknu ramecek
		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		style &= ~WS_OVERLAPPEDWINDOW;
		SetWindowLong(hwnd, GWL_STYLE, style);
	}

	void resizeWindowToFillScreen() {
		HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = {};
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(monitor, &mi);
		RECT r = mi.rcMonitor;

		SetWindowPos(
			hwnd, 
			nullptr, 
			r.left, 
			r.top, 
			r.right - r.left, 
			r.bottom - r.top,
			SWP_FRAMECHANGED | SWP_NOZORDER
		);
	}

	void DIB_clear(uint32_t color);
	void DIB_putPixel(int x, int y, uint32_t color);
	
	void DIB_drawTriangle(const Vec4& a, const Vec4& b, const Vec4& c, uint32_t color = 0xffffffff, bool useAntialiasing = false);
	void DIB_drawCircle(int cx, int cy, int r, uint32_t color)
	{
		for (int y = -r; y <= r; y++)
			for (int x = -r; x <= r; x++)
				if (x * x + y * y <= r * r)
					DIB_putPixel(cx + x, cy + y, color);
	}

	uint32_t getWidth() const { return static_cast<uint32_t>(width); }
	uint32_t getHeight() const { return static_cast<uint32_t>(height); }

private:
	
	HWND hwnd = nullptr;
	HINSTANCE hInstance = nullptr;
	//MainLoopCallback mainLoopProc;
	WindowCallback callback = nullptr;

	//HBITMAP hBitmap = nullptr;
	//uint32_t* framebuffer = nullptr;
	int width, height;

	void glibRegisterRawInputDevices();

};