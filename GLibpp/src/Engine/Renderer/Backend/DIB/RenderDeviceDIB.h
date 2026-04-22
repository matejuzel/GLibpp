#pragma once

#include "RasterizatorDIB.h"
#include "Color.h"
#include "VertexBuffer.h"
#include "WindowWin32.h"
#include "Win32Dc.h"

#include <vector>

namespace Render {

    // forward - kvuli pouziti friend
    template<typename D, typename T> class RenderDeviceBase;

    // forward
    class RenderDeviceDIB;
    class RenderTargetDIB;

    // Device traits
    template<>
    struct DeviceTraits<RenderDeviceDIB>
    {
        template<typename T>
        using GpuBuffer = std::vector<T>;

        using GpuBuffer3D = GpuBuffer<float>;
        using GpuBuffer2D = GpuBuffer<float>;
        using GpuIndexBuffer = GpuBuffer<uint32_t>;
    };

    // alias - schovame pred svetem - pouze pro interni zjednoduseni
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

        using GpuBuffer3D = DeviceTraits<Self>::GpuBuffer3D;
        using GpuBuffer2D = DeviceTraits<Self>::GpuBuffer2D;
        using GpuIndexBuffer = DeviceTraits<Self>::GpuIndexBuffer;

        VertexBuffer<Self> vertexBuffer;

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
            size_t size = target.descriptor.width * target.descriptor.height;
            std::fill_n(target.framebuffer, size, color);
            // dalo by se pouzit SSE (SIMD) pro rychlejší vyplnìní, ale pro jednoduchost a pøehlednost teï použijeme std::fill_n
        }

        void presentImpl(Target& target) noexcept
        {
            Win32DC wDc(window.getHwnd());
            BitBlt(
                wDc.get(),
                0, 0,
                target.descriptor.width, target.descriptor.height,
                target.getDC(),
                0, 0,
                SRCCOPY
            );


            /*
            uint32_t width = target.descriptor.width;
            uint32_t height = target.descriptor.height;
            window.presentToDc(width, height, target.getDC());
            */
        }

        void registerMeshImpl(const Mesh& mesh) noexcept
        {

            vertexBuffer.positions.push_back(3.14f);
            vertexBuffer.positions.push_back(4.0f);
            vertexBuffer.positions.push_back(54.2f);

            std::cout << "RegisterMesh(@todo)" << std::endl;
            for (auto v : vertexBuffer.positions) {

                std::cout << v << std::endl;
            }
            /*


            vertexBuffer.push_back(3.14159f);
            vertexBuffer.push_back(2.0f);
            vertexBuffer.push_back(3.23f);
            vertexBuffer.push_back(5.112f);
            vertexBuffer.push_back(7.23f);



            mesh.getIndexBuffer();
            */
        }


        Context createContextImpl() noexcept {
            Context ctx;
            return ctx;
        }

    };

}

