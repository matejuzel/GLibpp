#pragma once

#include <cstdint>
#include <windows.h>

class DIBFramebuffer {
public:
    const size_t width;
    const size_t height;
    const size_t bitDepth;

    DIBFramebuffer(size_t width, size_t height, size_t bitDepth = 32)
        : width(width), height(height), bitDepth(bitDepth) {
    }

    ~DIBFramebuffer() {
        if (memDC) DeleteDC(memDC);
        if (hBitmap) DeleteObject(hBitmap);
    }

    bool init(HWND hwnd);
    void present();

    uint32_t* data() { return framebuffer; }
    size_t getWidth() const { return width; }
    size_t getHeight() const { return height; }

private:
    HWND hwnd = nullptr;
    HBITMAP hBitmap = nullptr;
    HDC memDC = nullptr;
    uint32_t* framebuffer = nullptr;
};
