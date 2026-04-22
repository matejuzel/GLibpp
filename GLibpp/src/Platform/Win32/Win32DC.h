
#pragma once

#include <windows.h>

class Win32DC {

private:
    HWND hwnd;
    HDC hdc;

public:
    explicit Win32DC(HWND hwnd)
        : hwnd(hwnd), hdc(GetDC(hwnd)) {
    }

    ~Win32DC() {
        if (hdc) {
            ReleaseDC(hwnd, hdc);
        }
    }

    HDC get() const {
        return hdc;
    }

    // implicitní konverze (volitelné, ale praktické)
    operator HDC() const {
        return hdc;
    }

    // zakázat kopírování
    Win32DC(const Win32DC&) = delete;
    Win32DC& operator=(const Win32DC&) = delete;

    // povolit move
    Win32DC(Win32DC&& other) noexcept
        : hwnd(other.hwnd), hdc(other.hdc) {
        other.hdc = nullptr;
    }

    Win32DC& operator=(Win32DC&& other) noexcept {
        if (this != &other) {
            if (hdc) {
                ReleaseDC(hwnd, hdc);
            }
            hwnd = other.hwnd;
            hdc = other.hdc;
            other.hdc = nullptr;
        }
        return *this;
    }

};

