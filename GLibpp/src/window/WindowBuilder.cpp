#include <windows.h>
#include <string>
#include <iostream>
#include "WindowBuilder.h"
#include "core/App.h"
#include "core/input/Keymap.h"


WindowBuilder::WindowBuilder(MainLoopCallback mainLoopProc, WindowCallback proc)
{
    this->mainLoopProc = mainLoopProc;
    this->callback = proc;
}

bool WindowBuilder::init(int width, int height) {

    this->width = width;
    this->height = height;

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
        this->width, this->height,
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

bool WindowBuilder::run(float logicHz)
{
    return this->mainLoopProc(logicHz);
}

HWND WindowBuilder::getHwnd() const {
    return this->hwnd;
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

void WindowBuilder::consoleSetFixedViewport()
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

void WindowBuilder::consolePrint(std::string text)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { 0, 0 };
    SetConsoleCursorPosition(h, pos);

    std::cout << text;
}

void WindowBuilder::consoleClear()
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

void WindowBuilder::DIB_init()
{

    this->width = width;
    this->height = height;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down bitmap
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(nullptr);

    hBitmap = CreateDIBSection(
        screen,
        &bmi,
        DIB_RGB_COLORS,
        (void**)&framebuffer,
        nullptr,
        0
    );

    ReleaseDC(nullptr, screen);
}

void WindowBuilder::DIB_clear(uint32_t color)
{
    for (int i = 0; i < width * height; i++)
        framebuffer[i] = color;
}

void WindowBuilder::DIB_putPixel(int x, int y, uint32_t color)
{
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    framebuffer[y * width + x] = color;
}

void WindowBuilder::DIB_drawBitmap()
{
    HDC dc = GetDC(this->hwnd);

    HDC memDC = CreateCompatibleDC(dc);
    HBITMAP old = (HBITMAP)SelectObject(memDC, hBitmap);

    BitBlt(dc, 0, 0, this->width, this->height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, old);
    DeleteDC(memDC);

    ReleaseDC(hwnd, dc);
}
