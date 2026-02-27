#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include "math/Vec4.h"

class WindowBuilder {

public:

	using WindowCallback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
	//using MainLoopCallback = bool(*)(float logicHz);

	WindowBuilder(int width, int height, WindowCallback proc);

	bool build();
	HWND getHwnd() const;

	static void consoleSetFixedViewport();
	static void consolePrint(std::string text);
	static void consoleClear();

	void DIB_init();
	void DIB_clear(uint32_t color);
	void DIB_putPixel(int x, int y, uint32_t color);
	void DIB_drawBitmap();
	void DIB_drawTriangle(const Vec4& a, const Vec4& b, const Vec4& c, uint32_t color = 0xffffffff);
	void DIB_drawCircle(int cx, int cy, int r, uint32_t color)
	{
		for (int y = -r; y <= r; y++)
			for (int x = -r; x <= r; x++)
				if (x * x + y * y <= r * r)
					DIB_putPixel(cx + x, cy + y, color);
	}



private:
	
	HWND hwnd;
	HINSTANCE hInstance;
	//MainLoopCallback mainLoopProc;
	WindowCallback callback = nullptr;

	HBITMAP hBitmap;
	uint32_t* framebuffer;
	int width, height;

	void glibRegisterRawInputDevices();

};