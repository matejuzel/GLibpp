#pragma once

#include "IRenderTarget.h"
#include "RenderTargetDescriptor.h"
#include <windows.h>
#include <cstdint>
#include <cstring>

class RenderTargetDIB : public IRenderTarget {
private:

	HBITMAP hBitmap = nullptr;
    HDC memDC = nullptr;
	HGDIOBJ oldBitmap = nullptr;
	uint32_t* framebuffer = nullptr;

public:

    RenderTargetDIB(const RenderTargetDescriptor& descriptor): IRenderTarget(descriptor) 
    {

        if (descriptor.width < 1 || descriptor.height < 1) {
            throw std::runtime_error(
                "Invalid RenderTarget dimensions: [" +
                std::to_string(descriptor.width) + "x" +
                std::to_string(descriptor.height) + "]"
            );
        }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = int(descriptor.width);
        bmi.bmiHeader.biHeight = -int(descriptor.height);
        bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = int(32); // @todo: mapovat z TextureFormat
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

        if (!hBitmap || !framebuffer) 
        {
            ReleaseDC(nullptr, screen);

            {
                // ziskani msg
                DWORD err = GetLastError();
                LPVOID buf;
                FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, err, 0, (LPSTR)&buf, 0, nullptr
                );
                std::string msg = (char*)buf;
                LocalFree(buf);
                throw std::runtime_error("Failed to create DIB section. GetLastError Message: " + msg);
            }
            
        }

		ReleaseDC(nullptr, screen);

        memDC = CreateCompatibleDC(nullptr);
        oldBitmap = SelectObject(memDC, hBitmap);

    }

    ~RenderTargetDIB() 
    {

        SelectObject(memDC, oldBitmap);

        if (hBitmap) {
            DeleteObject(hBitmap);
            hBitmap = nullptr;
        }

        if (memDC) {
            DeleteDC(memDC);
            memDC = nullptr;
		}
	}
    
	HDC getDC() const { return memDC; }
	HBITMAP getBitmap() const { return hBitmap; }
	uint32_t* getFramebuffer() const { return framebuffer; }


    void putPixel(uint32_t x, uint32_t y, uint32_t color) noexcept {

        uint32_t width = descriptor.width;
        uint32_t height = descriptor.height;

        if (x >= width || y >= height)
            return;

        framebuffer[y * width + x] = color;
    }


    


};