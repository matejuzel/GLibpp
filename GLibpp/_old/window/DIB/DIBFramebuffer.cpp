#include "window/DIB/DIBFramebuffer.h"

bool DIBFramebuffer::init(HWND hwnd) {
    this->hwnd = hwnd;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = int(width);
    bmi.bmiHeader.biHeight = -int(height);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = int(bitDepth);
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

    if (!hBitmap || !framebuffer)
        return false;

    memDC = CreateCompatibleDC(nullptr);
    SelectObject(memDC, hBitmap);

    return true;
}

void DIBFramebuffer::present() {
    HDC dc = GetDC(hwnd);
    BitBlt(dc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
    ReleaseDC(hwnd, dc);
}
