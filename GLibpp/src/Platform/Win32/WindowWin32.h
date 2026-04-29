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
	HDC hdc = nullptr;
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
		if (hwnd && hdc)
		{
			ReleaseDC(hwnd, hdc);
			hdc = nullptr;
		}

		if (hwnd) 
		{
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

	bool build(std::wstring preferedMonitor = L"");
	
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

	HDC getHDC() const noexcept { return hdc; }

	void releaseDeviceContext(HDC hdc) const {
		ReleaseDC(hwnd, hdc);
	}

	struct MonitorData {
		HMONITOR handle;      // Unikátní identifikátor ve Win32
		std::wstring name;    // Systémový název (napø. pro ukládání do configu)
		int width;
		int height;
		int x;                // Souøadnice v rámci virtuální plochy
		int y;
		bool isPrimary;

		friend std::wostream& operator<<(std::wostream& os, const MonitorData& md) {
			os << "Monitor: " << md.name.c_str() << " (" << md.width << "x" << md.height << ") at (" << md.x << "," << md.y << ")";
			if (md.isPrimary) os << " [PRIMARY]";
			return os;
		}
	};

	std::vector<MonitorData> getMonitorInfoList() 
	{
		std::vector<MonitorData> monitors;

		auto callback = [](HMONITOR hMonitor, HDC hdc, LPRECT lprc, LPARAM data) -> BOOL {
			auto* list = reinterpret_cast<std::vector<MonitorData>*>(data);

			MONITORINFOEXW mi;
			mi.cbSize = sizeof(mi);

			if (GetMonitorInfoW(hMonitor, &mi)) {
				MonitorData d;
				d.handle = hMonitor;
				d.name = mi.szDevice;
				d.width = mi.rcMonitor.right - mi.rcMonitor.left;
				d.height = mi.rcMonitor.bottom - mi.rcMonitor.top;
				d.x = mi.rcMonitor.left;
				d.y = mi.rcMonitor.top;
				d.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY);

				list->push_back(d);
			}
			return TRUE;
			};

		EnumDisplayMonitors(NULL, NULL, callback, reinterpret_cast<LPARAM>(&monitors));
		return monitors;
	}

	void moveToMonitor(HWND hwnd, HMONITOR hMonitor) {
		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);

		// Získáme informace o monitoru na základì jeho identifikátoru (handle)
		if (GetMonitorInfo(hMonitor, &mi)) {
			// mi.rcMonitor obsahuje souøadnice celého monitoru ve virtuální ploše
			SetWindowPos(hwnd, NULL,
				mi.rcMonitor.left,  // Toto definuje, na kterém monitoru okno skonèí
				mi.rcMonitor.top,
				0, 0,
				SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
	}

private:

	bool RegisterWindowClass(HINSTANCE hInstance);
	LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
	void setHwnd(HWND hwnd) { this->hwnd = hwnd; }
};