#include <windows.h>
#include "WindowBuilder.h"
#include "core/App.h"
#include "core/input/Keymap.h"


WindowBuilder::WindowBuilder(MainLoopCallback mainLoopProc, WindowCallback proc)
{
    this->mainLoopProc = mainLoopProc;
    this->callback = proc;
}

bool WindowBuilder::init() {

    hInstance = GetModuleHandle(nullptr);

    const wchar_t CLASS_NAME[] = L"MyWinAPIWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = this->callback;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    this->hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Moje WinAPI okno",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (this->hwnd == nullptr) return false;

    this->glibRegisterRawInputDevices();

    ShowWindow(this->hwnd, SW_SHOW);

    return true;
}

bool WindowBuilder::run()
{
    return this->mainLoopProc();
}

void WindowBuilder::glibRegisterRawInputDevices()
{
    // registrace Raw Input Devices - pro lepsi odchytavani stisku klaves

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06; // Keyboard
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = this->hwnd;

    RegisterRawInputDevices(&rid, 1, sizeof(rid));
}
