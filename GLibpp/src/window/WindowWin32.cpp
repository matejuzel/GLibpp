#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <iostream>
#include "WindowWin32.h"
#include "core/App.h"
#include "core/input/Keymap.h"

 bool WindowWin32::RegisterWindowClass(HINSTANCE hInstance, WindowCallback callback) {

    // staticky atribut deklarovany v teto staticke metode se provede pouze pri prvnim volani metody (function-local static v C++)
    // zaroven v C++11+ by to melo byt thread safe - takze neni potreba resit atomic
    static bool registered = false; 
    if (registered) return true;

    WNDCLASS wc = {};
    wc.lpfnWndProc = callback;
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
    hInstance = GetModuleHandle(nullptr);

    //const wchar_t CLASS_NAME[] = L"MyWinAPIWindowClass";

    if (!RegisterWindowClass(hInstance, callback))
    {
        std::cerr << "RegisterClass failed: " << GetLastError() << std::endl;
        return false;
    }

    // timto zjsitime jak velke se ma okno vytvorit (klientska oblast + vse ostatni jako okraje, ramecek, lista, ...)
    RECT rect = { 0, 0, width, height };
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;

    this->hwnd = CreateWindowEx(
        0,
        GetClassName(),
        L"Moje WinAPI okno",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winWidth, winHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr
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

void WindowWin32::consoleSetFixedViewport()
{
    // nastavime konzoli na fixed viewport (vypne scrollovani)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFOEX infoConsole = { sizeof(infoConsole) };
    GetConsoleScreenBufferInfoEx(h, &infoConsole);

    infoConsole.dwSize.Y = infoConsole.srWindow.Bottom - infoConsole.srWindow.Top + 1;

    SetConsoleScreenBufferInfoEx(h, &infoConsole);

    // vypneme kurzor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(h, &cursorInfo);

    cursorInfo.bVisible = FALSE;
    
    SetConsoleCursorInfo(h, &cursorInfo);
}

void WindowWin32::consolePrint(std::string text)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { 0, 0 };
    SetConsoleCursorPosition(h, pos);

    std::cout << text;
}

void WindowWin32::consoleClear()
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);

    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD written;

    FillConsoleOutputCharacter(h, ' ', cellCount, {0, 0}, &written);
    FillConsoleOutputAttribute(h, csbi.wAttributes, cellCount, {0, 0}, &written);
    SetConsoleCursorPosition(h, {0, 0});
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

