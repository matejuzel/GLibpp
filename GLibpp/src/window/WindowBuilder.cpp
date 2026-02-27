#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <iostream>
#include "WindowBuilder.h"
#include "core/App.h"
#include "core/input/Keymap.h"


WindowBuilder::WindowBuilder(int width, int height, WindowCallback proc)
{
    this->callback = proc;
    this->width = width;
    this->height = height;
}

bool WindowBuilder::build() 
{
    hInstance = GetModuleHandle(nullptr);

    const wchar_t CLASS_NAME[] = L"MyWinAPIWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = this->callback;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    ATOM cls = RegisterClass(&wc);
    if (cls == 0) {
        std::cerr << "RegisterClass failed: " << GetLastError() << std::endl;
        return false;
    }
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

    if (this->hwnd == nullptr) {
        std::cerr << "CreateWindowEx failed: " << GetLastError() << std::endl;
        return false;
    }

    this->glibRegisterRawInputDevices();

    ShowWindow(this->hwnd, SW_SHOWNORMAL);
    UpdateWindow(this->hwnd);

    return true;
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

bool WindowBuilder::DIB_init()
{
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

    return true;
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

// jednoduchá pomocná funkce pro blend (50%)
// vstupní formát pøedpokládá 0xAARRGGBB
static inline uint32_t blend50(uint32_t dst, uint32_t src)
{
    uint8_t sr = (src >> 16) & 0xFF;
    uint8_t sg = (src >> 8) & 0xFF;
    uint8_t sb = (src) & 0xFF;

    uint8_t dr = (dst >> 16) & 0xFF;
    uint8_t dg = (dst >> 8) & 0xFF;
    uint8_t db = (dst) & 0xFF;

    uint8_t r = (uint8_t)((sr + dr) / 2);
    uint8_t g = (uint8_t)((sg + dg) / 2);
    uint8_t b = (uint8_t)((sb + db) / 2);

    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

// Bresenham line algorithm
static void drawLine(uint32_t* framebuffer, int fbWidth, int fbHeight, int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        if (x0 >= 0 && y0 >= 0 && x0 < fbWidth && y0 < fbHeight)
            framebuffer[y0 * fbWidth + x0] = color;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Pomocné funkce pro Wuùv algoritmus
static inline int ipart(float x) { return (int)std::floor(x); }
static inline float fpart(float x) { return x - std::floor(x); }
static inline float rfpart(float x) { return 1.0f - fpart(x); }

// Blend src color over dst with given coverage (0.0 .. 1.0). Vrací výslednou 0xAARRGGBB (alpha nastavíme na 0xFF).
static inline uint32_t blendWithCoverage(uint32_t dst, uint32_t src, float coverage)
{
    coverage = std::max(0.0f, std::min(1.0f, coverage));
    uint8_t sr = (src >> 16) & 0xFF;
    uint8_t sg = (src >> 8) & 0xFF;
    uint8_t sb = (src) & 0xFF;

    uint8_t dr = (dst >> 16) & 0xFF;
    uint8_t dg = (dst >> 8) & 0xFF;
    uint8_t db = (dst) & 0xFF;

    uint8_t r = (uint8_t)(sr * coverage + dr * (1.0f - coverage));
    uint8_t g = (uint8_t)(sg * coverage + dg * (1.0f - coverage));
    uint8_t b = (uint8_t)(sb * coverage + db * (1.0f - coverage));

    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

// Xiaolin Wu antialiased line algorithm
static void drawLineWithAntialiasing(uint32_t* framebuffer, int fbWidth, int fbHeight, int x0, int y0, int x1, int y1, uint32_t color)
{
    // Implementace upravená pro integer framebuffer s 0xAARRGGBB komponentami.
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    float dx = (float)(x1 - x0);
    float dy = (float)(y1 - y0);
    float gradient = (dx == 0.0f) ? 1.0f : (dy / dx);

    // handle first endpoint
    float xend = std::round((float)x0);
    float yend = y0 + gradient * (xend - x0);
    float xgap = rfpart((float)x0 + 0.5f);
    int xpxl1 = (int)xend;
    int ypxl1 = ipart(yend);

    auto plot = [&](int x, int y, float c) {
        if (c <= 0.0f) return;
        if (c > 1.0f) c = 1.0f;
        if (steep) {
            // swapped back: x -> y, y -> x
            if (x >= 0 && x < fbHeight && y >= 0 && y < fbWidth) {
                size_t idx = (size_t)x * fbWidth + y;
                framebuffer[idx] = blendWithCoverage(framebuffer[idx], color, c);
            }
        }
        else {
            if (x >= 0 && x < fbWidth && y >= 0 && y < fbHeight) {
                size_t idx = (size_t)y * fbWidth + x;
                framebuffer[idx] = blendWithCoverage(framebuffer[idx], color, c);
            }
        }
        };

    plot(xpxl1, ypxl1, rfpart(yend) * xgap);
    plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
    float intery = yend + gradient;

    // handle second endpoint
    xend = std::round((float)x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fpart((float)x1 + 0.5f);
    int xpxl2 = (int)xend;
    int ypxl2 = ipart(yend);

    plot(xpxl2, ypxl2, rfpart(yend) * xgap);
    plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap);

    // main loop
    for (int x = xpxl1 + 1; x <= xpxl2 - 1; x++) {
        plot(x, ipart(intery), rfpart(intery));
        plot(x, ipart(intery) + 1, fpart(intery));
        intery = intery + gradient;
    }
}

void WindowBuilder::DIB_drawTriangle(const Vec4& a, const Vec4& b, const Vec4& c, uint32_t color, bool useAntialiasing)
{
    int ax = (int)std::lround(a.x);
    int ay = (int)std::lround(a.y);
    int bx = (int)std::lround(b.x);
    int by = (int)std::lround(b.y);
    int cx = (int)std::lround(c.x);
    int cy = (int)std::lround(c.y);

    if (useAntialiasing) 
    {
        drawLineWithAntialiasing(framebuffer, this->width, this->height, ax, ay, bx, by, color);
        drawLineWithAntialiasing(framebuffer, this->width, this->height, bx, by, cx, cy, color);
        drawLineWithAntialiasing(framebuffer, this->width, this->height, cx, cy, ax, ay, color);
    } 
    else 
    {
        drawLine(framebuffer, this->width, this->height, ax, ay, bx, by, color);
        drawLine(framebuffer, this->width, this->height, bx, by, cx, cy, color);
        drawLine(framebuffer, this->width, this->height, cx, cy, ax, ay, color);
    }

    
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
