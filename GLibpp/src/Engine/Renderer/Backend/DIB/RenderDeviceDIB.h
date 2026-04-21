#pragma once

#include "RasterizatorDIB.h"
#include "Color.h"

// forward - kvuli pouziti friend
template<typename D, typename T>
class RenderDeviceBase;

class RenderDeviceDIB;
class RenderTargetDIB;

namespace internal {
    using RenderDeviceDIBBase = RenderDeviceBase<RenderDeviceDIB, RenderTargetDIB>;
};

class RenderDeviceDIB : public internal::RenderDeviceDIBBase
{
    template<typename D, typename T>
    friend class RenderDeviceBase;   // Base má pøístup do private Derived ...Impl(), ktere nemaji byt videt zvenci

private:

    using Self = RenderDeviceDIB;
    using Base = internal::RenderDeviceDIBBase;

public:

    RenderDeviceDIB(WindowWin32& window) : Base(window) {}

private:

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
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        );

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
            Color::Grayscale(0.5f).toRGBA()
        );
    }

    void clearImpl(const Context& ctx, Target& target) noexcept
    {
        uint32_t color = ctx.clearColor.toRGBA();
        int size = target.descriptor.width * target.descriptor.height;
        std::fill_n(target.framebuffer, size, color);
        // dalo by se pouzit SSE (SIMD) pro rychlejší vyplnìní, ale pro jednoduchost a pøehlednost teï použijeme std::fill_n
    }

    void presentImpl(Target& target) noexcept
    {
        HDC windowDC = GetDC(window.getHwnd());

        BitBlt(
            windowDC,
            0, 0,
            target.descriptor.width, target.descriptor.height,
            (static_cast<const Target&>(target)).getDC(),
            0, 0,
            SRCCOPY
        );

        ReleaseDC(window.getHwnd(), windowDC);
    }

    Context createContextImpl() noexcept {
        Context ctx;
        return ctx;
    }

};