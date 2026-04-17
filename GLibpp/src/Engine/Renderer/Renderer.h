#pragma once

#include "RenderTargetDescriptor.h"
#include "Viewport.h"
#include "Color.h"
#include "WindowWin32.h"

#include <cstdint>
#include <windows.h>
#include <memory>
#include "Vec4.h"
#include "Mtx4.h"

template<typename Device, typename Target>
class RenderContext {

public:
    RenderContext() = default;
    ~RenderContext() = default;

    Target* target = nullptr;
    Viewport viewport = { 0,0,800,600 };
    Mtx4 view = Mtx4::Identity();
    Mtx4 projection = Mtx4::Identity();
    Color clearColor = { 0,0,0,255 };
    int frameCnt = 0;
    //typename Device::MeshHandle mesh = { 0 };
    //typename Device::MaterialHandle material = { 0 };
};

template<typename Device, typename DerivedTarget>
class RenderTargetBase 
{
public:

    RenderTargetDescriptor descriptor;

    RenderTargetBase(const RenderTargetDescriptor& descriptor) :descriptor(descriptor) {}
    ~RenderTargetBase() = default;
};


template <typename DerivedDevice, typename DerivedTarget>
class RenderDeviceBase
{
public:

    using Context = RenderContext<DerivedDevice, DerivedTarget>;

    void draw(const Context& ctx, DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->drawImpl(ctx, target);
    }

    void clear(const Context& ctx, DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->clearImpl(ctx, target);
    }

    std::unique_ptr<DerivedTarget> createTarget(const RenderTargetDescriptor &descriptor) noexcept
    {
        return static_cast<DerivedDevice*>(this)->createTargetImpl(descriptor);
    }

    void present(Context& ctx, const DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->presentImpl(ctx, target);
    }
};


class RenderDeviceDIB; // forward

class RenderTargetDIB : public RenderTargetBase<RenderDeviceDIB, RenderTargetDIB>
{
public:
    HBITMAP hBitmap = nullptr;
    HDC memDC = nullptr;
    HGDIOBJ oldBitmap = nullptr;
    uint32_t* framebuffer = nullptr;

    RenderTargetDIB(const RenderTargetDescriptor& descriptor)
        : RenderTargetBase<RenderDeviceDIB, RenderTargetDIB>(descriptor)
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

    void inline putPixel(uint32_t x, uint32_t y, uint32_t color) noexcept
    {

        uint32_t width = descriptor.width;
        uint32_t height = descriptor.height;

        if (x >= width || y >= height)
            return;

        framebuffer[y * width + x] = color;
    }

};

class RasterizatorDIB {

public:

    static void inline drawLine(RenderTargetDIB& target, int x0, int y0, int x1, int y1, uint32_t color) noexcept
    {
        int dx = abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true)
        {
            target.putPixel(x0, y0, color);

            if (x0 == x1 && y0 == y1)
                break;

            int e2 = 2 * err;

            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }

            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    static void inline drawTriangle(RenderTargetDIB& target, int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) noexcept
    {
        // Seøadíme vrcholy podle Y (od nejnižšího)
        if (y1 < y0) { std::swap(y0, y1); std::swap(x0, x1); }
        if (y2 < y0) { std::swap(y0, y2); std::swap(x0, x2); }
        if (y2 < y1) { std::swap(y1, y2); std::swap(x1, x2); }

        auto drawSpan = [&](int y, int xStart, int xEnd)
            {
                if (y < 0 || y >= (int)target.descriptor.height)
                    return;

                if (xStart > xEnd)
                    std::swap(xStart, xEnd);

                xStart = std::max(0, xStart);
                xEnd = std::min((int)target.descriptor.width - 1, xEnd);

                uint32_t* row = target.framebuffer + y * target.descriptor.width;
                for (int x = xStart; x <= xEnd; x++)
                    row[x] = color;
            };

        auto edgeInterp = [&](int y, int x0, int y0, int x1, int y1)
            {
                if (y1 == y0)
                    return x0;

                return x0 + (x1 - x0) * (y - y0) / (y1 - y0);
            };

        // Horní èást (od y0 do y1)
        for (int y = y0; y <= y1; y++)
        {
            int xa = edgeInterp(y, x0, y0, x2, y2);
            int xb = edgeInterp(y, x0, y0, x1, y1);
            drawSpan(y, xa, xb);
        }

        // Dolní èást (od y1 do y2)
        for (int y = y1; y <= y2; y++)
        {
            int xa = edgeInterp(y, x0, y0, x2, y2);
            int xb = edgeInterp(y, x1, y1, x2, y2);
            drawSpan(y, xa, xb);
        }
    }

    static void inline drawQuad(RenderTargetDIB& target, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) noexcept
    {
        drawTriangle(target, x0, y0, x1, y1, x2, y2, color);
        drawTriangle(target, x0, y0, x2, y2, x3, y3, color);
    }

    static void inline drawQuad(RenderTargetDIB& target, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color) noexcept
    {
        drawQuad(target,
            static_cast<int>(x0),
            static_cast<int>(y0),
            static_cast<int>(x1),
            static_cast<int>(y1),
            static_cast<int>(x2),
            static_cast<int>(y2),
            static_cast<int>(x3),
            static_cast<int>(y3),
            color
        );
    }


};

class RenderDeviceDIB : public RenderDeviceBase<RenderDeviceDIB, RenderTargetDIB>
{
public:

    using Device = RenderDeviceDIB;
    using Target = RenderTargetDIB;
    using Context = RenderContext<Device, Target>;
    using Backend = RenderDeviceBase<Device, Target>;

    HWND hwnd;

    RenderDeviceDIB(HWND hwnd) : hwnd(hwnd) {}

//protected:
    void drawImpl(const Context& ctx, Target& target) noexcept
    {

        int verts[4][2] = {
            {-1,-1},
            { 1,-1},
            { 1, 1},
            {-1, 1},
        };

        Vec4 va(verts[0][0], verts[0][1], 0, 1);
        Vec4 vb(verts[1][0], verts[1][1], 0, 1);
        Vec4 vc(verts[2][0], verts[2][1], 0, 1);
        Vec4 vd(verts[3][0], verts[3][1], 0, 1);

        Mtx4 model(
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        );
        model.rotateY(0.001f * ctx.frameCnt);

        Mtx4 mvp = ctx.projection * ctx.view * model;

        va = mvp * va;
        vb = mvp * vb;
        vc = mvp * vc;
        vd = mvp * vd;

        va.divideW();
        vb.divideW();
        vc.divideW();
        vd.divideW();

        uint32_t x = ctx.viewport.x;
        uint32_t y = ctx.viewport.y;
        uint32_t width = ctx.viewport.width;
        uint32_t height = ctx.viewport.height;

        auto viewportTransform = [&](Vec4& v) {
            v.x = (v.x * 0.5f + 0.5f) * width + x;
            v.y = (-v.y * 0.5f + 0.5f) * height + y;
            };
        viewportTransform(va);
        viewportTransform(vb);
        viewportTransform(vc);
        viewportTransform(vd);

        // FRONT (+)
        RasterizatorDIB::drawQuad(
            target,
            va.x, va.y,
            vb.x, vb.y,
            vc.x, vc.y,
            vd.x, vd.y,
            0xffff00ff
        );
    }

    void clearImpl(const Context& ctx, Target& target) noexcept
    {
        uint32_t color = ctx.clearColor.toRGBA();
        int size = target.descriptor.width * target.descriptor.height;
        std::fill_n(target.framebuffer, size, color);
        // dalo by se pouzit SSE (SIMD) pro rychlejší vyplnìní, ale pro jednoduchost a pøehlednost teï použijeme std::fill_n
    }

    void presentImpl(Context& ctx, const Target& target) noexcept
    {
        HDC windowDC = GetDC(hwnd);
        
        BitBlt(
            windowDC,
            0, 0,
            target.descriptor.width, target.descriptor.height,
            (static_cast<const RenderTargetDIB&>(target)).getDC(),
            0, 0,
            SRCCOPY
        );
        

        ReleaseDC(hwnd, windowDC);

        ctx.frameCnt += 1;
    }

    std::unique_ptr<RenderTargetDIB> createTargetImpl(const RenderTargetDescriptor& descriptor) noexcept
    {
        return std::make_unique<RenderTargetDIB>(descriptor);
    }
};

template <typename Device>
class Renderer {
private:

    using Context = RenderContext<Device, typename Device::Target>;

    std::unique_ptr<Device> device;
    std::unique_ptr<typename Device::Target> target;

    const WindowWin32& window;

    Context ctx{};

public:
    Renderer(const WindowWin32& window)
        : device(std::make_unique<Device>(window.getHwnd()))
        , window(window)
    {
        target = device->createTarget(RenderTargetDescriptor::Framebuffer(
            window.getClientWidth(),
            window.getClientHeight()
        ));

        
        
    }

    void renderFrame()
    {
        
        ctx.target = target.get();

        ctx.view = Mtx4::LookAt(
            Vec4(5,5,5,1),
            Vec4(0,0,0,1),
            Vec4(0,1,0,0)
        );

        ctx.projection = Mtx4::Perspective(
            45.0f,
            (float)window.getClientWidth() / (float)window.getClientHeight(),
            0.01f,
            100.0f
        );

        ctx.viewport = {
            0,0,
            window.getClientWidth(),
            window.getClientHeight()
        };

        device->clear(ctx, *target);
        device->draw(ctx, *target);
        device->present(ctx, *target);
    }

    void resize(uint32_t width, uint32_t height)
    {
        target = device->createTarget(RenderTargetDescriptor::Framebuffer(width, height));
    }
};
