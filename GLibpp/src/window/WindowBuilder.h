#pragma once
#include <windows.h>
#include <string>

class WindowBuilder {

public:

	using WindowCallback = LRESULT(CALLBACK*)(HWND, UINT, WPARAM, LPARAM);
	using MainLoopCallback = bool(*)(float logicHz);

	WindowBuilder(MainLoopCallback mainLoopProc, WindowCallback proc);

	bool init(int width, int height);
	bool run(float logicHz);
	HWND getHwnd() const;

	static void consoleSetFixedViewport();
	static void consolePrint(std::string text);
	static void consoleClear();

	void DIB_init();
	void DIB_clear(uint32_t color);
	void DIB_putPixel(int x, int y, uint32_t color);
	void DIB_drawBitmap();
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
	MainLoopCallback mainLoopProc;
	WindowCallback callback = nullptr;

	HBITMAP hBitmap;
	uint32_t* framebuffer;
	int width, height;

	void glibRegisterRawInputDevices();

};