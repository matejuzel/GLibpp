#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <iostream>
#include "WindowWin32.h"
#include "core/App.h"
#include "core/input/Keymap.h"

void WindowWin32::setKeyCallback(std::function<void(KeyMap, bool)> cb) noexcept
{
    onKeyEvent = std::move(cb);
}

void WindowWin32::setOnCloseCallback(std::function<void()> cb) noexcept
{
    onClose = std::move(cb);
}

void WindowWin32::waitEvents() const noexcept
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void WindowWin32::pollEvents() const noexcept
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT WindowWin32::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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

bool WindowWin32::RegisterWindowClass(HINSTANCE hInstance) {

    // staticky atribut deklarovany v teto staticke metode se provede pouze pri prvnim volani metody (function-local static v C++)
    // zaroven v C++11+ by to melo byt thread safe - takze neni potreba resit atomic
    static bool registered = false; 
    if (registered) return true;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = GetClassName();

    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    ATOM cls = RegisterClass(&wc);
    if (cls == 0) return false;

    registered = true;

    return true;
}

bool WindowWin32::build()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    if (!RegisterWindowClass(hInstance))
    {
        std::cerr << "RegisterClass failed: " << GetLastError() << std::endl;
        return false;
    }

    // timto zjsitime jak velke se ma okno vytvorit (klientska oblast + vse ostatni jako okraje, ramecek, lista, ...)
    RECT rect = { 0, 0, width, height };
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;

    CreateWindowEx(
        0,
        GetClassName(),
        L"Moje WinAPI okno",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winWidth, winHeight,
        nullptr,
        nullptr,
        hInstance,
        this
    );


    if (this->hwnd == nullptr) {
        std::cerr << "CreateWindowEx failed: " << GetLastError() << std::endl;
        return false;
    }

    this->glibRegisterRawInputDevices();

    auto winShowMode = SW_SHOWNORMAL;
    if (maximized) winShowMode = SW_MAXIMIZE;

    ShowWindow(this->hwnd, winShowMode);
    UpdateWindow(this->hwnd);

    return true;
}

HWND WindowWin32::getHwnd() const {
    return this->hwnd;
}

void WindowWin32::glibRegisterRawInputDevices()
{
    // registrace Raw Input Devices - pro lepsi odchytavani stisku klaves

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06; // Keyboard
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = this->hwnd;

    RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void WindowWin32::removeOverlapProperty() 
{
    // odebere oknu ramecek
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~WS_OVERLAPPEDWINDOW;
    SetWindowLong(hwnd, GWL_STYLE, style);
}

void WindowWin32::resizeWindowToFillScreen() 
{
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

LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WindowWin32* window = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<WindowWin32*>(cs->lpCreateParams);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
        window->setHwnd(hwnd);
    }
    else
    {
        window = reinterpret_cast<WindowWin32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (window) return window->handleMessage(msg, wParam, lParam);

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
