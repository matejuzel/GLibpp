#pragma once

#include "DeviceTargetBase.h"
#include "RenderTargetDescriptor.h"

#include <vector>

namespace GLibpp::Render {

    class DeviceDIB; // forward
    class DeviceTargetDIB; // forward

    namespace internal {
        using RenderTargetDIBBase = DeviceTargetBase<DeviceDIB>;
    };

    class DeviceTargetDIB : public internal::RenderTargetDIBBase
    {
    private:
        using Self = DeviceTargetDIB;
        using Base = internal::RenderTargetDIBBase;

    public:
        // --- barevny target: DIB sekce, kterou umi BitBlt ---
        HBITMAP hBitmap = nullptr;
        HDC memDC = nullptr;
        HGDIOBJ oldBitmap = nullptr;
        uint32_t* framebuffer = nullptr;

        // --- depth target: souvisle pole hloubek, zadne GDI ---
        // (hloubku nikdo neblituje, takze DIB sekce, HDC ani HBITMAP nemaji smysl;
        //  u barevneho targetu je tenhle vektor prazdny a naopak)
        std::vector<float> depthbuffer;

        // --- textura (ShaderResource): texely v pameti, zadne GDI ---
        // framebuffer ukazuje do tohoto vektoru, takze pristup k pixelum je
        // jednotny s barevnym targetem (putPixel, primy pointer pro sampling)
        std::vector<uint32_t> texelStorage;

        DeviceTargetDIB() = default;

        // vlastni GDI handly (a buffer) -> kopie by je uvolnila dvakrat
        DeviceTargetDIB(const DeviceTargetDIB&) = delete;
        DeviceTargetDIB& operator=(const DeviceTargetDIB&) = delete;
        DeviceTargetDIB(DeviceTargetDIB&&) = delete;
        DeviceTargetDIB& operator=(DeviceTargetDIB&&) = delete;

        DeviceTargetDIB(const RenderTargetDescriptor& descriptor)
            : Base(descriptor)
        {

            if (descriptor.width < 1 || descriptor.height < 1) {
                throw std::runtime_error(
                    "Invalid RenderTarget dimensions: [" +
                    std::to_string(descriptor.width) + "x" +
                    std::to_string(descriptor.height) + "]"
                );
            }

            // depth target: jen alokace pole, GDI cesta se preskoci
            if (isDepthFormat(descriptor.format)) {
                depthbuffer.assign(pixelCount(), 0.0f);
                return;
            }

            // textura (ShaderResource): texely v pameti, GDI cesta se preskoci;
            // pixely nahraje backend pri textureRegister (upload walk)
            if (descriptor.usage == TextureUsage::ShaderResource) {
                texelStorage.assign(pixelCount(), 0u);
                framebuffer = texelStorage.data();
                return;
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

        ~DeviceTargetDIB()
        {
            // depth target zadne GDI nema - memDC je nullptr a SelectObject by se volal naprazdno
            if (memDC) SelectObject(memDC, oldBitmap);

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

        size_t pixelCount() const noexcept { return size_t(descriptor.width) * descriptor.height; }

        // hloubkovy target (bez DIB sekce) vs barevny - rozhoduje format v descriptoru
        bool isDepth() const noexcept { return isDepthFormat(descriptor.format); }

        // true az kdyz je target skutecne pouzitelny jako depth buffer daneho rozliseni
        bool isDepthUsable() const noexcept { return isDepth() && depthbuffer.size() == pixelCount(); }

        // textura pro sampling (ShaderResource) - nema DIB sekci ani DC
        bool isShaderResource() const noexcept { return descriptor.usage == TextureUsage::ShaderResource; }

        void inline putPixel(uint32_t x, uint32_t y, uint32_t color) noexcept
        {

            uint32_t width = descriptor.width;
            uint32_t height = descriptor.height;

            if (x >= width || y >= height)
                return;

            framebuffer[y * width + x] = color;
        }

    };

}