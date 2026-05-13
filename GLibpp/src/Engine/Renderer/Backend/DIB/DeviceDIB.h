#pragma once

#include "RasterizatorDIB.h"
#include "Color.h"
#include "DeviceBase.h"
#include "VertexBuffer.h"
#include "WindowWin32.h"
#include "Win32Dc.h"

#include <vector>
#include <immintrin.h> // AVX2

namespace Render {

    
    template<typename Device>
    struct RegistryDIB
    {
        VertexBuffer<Device> vertexBuffer;

        typename Device::TargetRegistry targets;

        //StableRegistry<DeviceTargetBase<Device>> targets;
        // ..
    };


    enum class ClearMode {
        Scalar,
        SSE2,
        AVX2
    };

    struct CpuFeatures {
        bool sse2 = false;
        bool avx2 = false;
    };

    inline CpuFeatures detectCpuFeatures() {
        CpuFeatures f;

        int cpuInfo[4];

        __cpuid(cpuInfo, 1);
        f.sse2 = (cpuInfo[3] & (1 << 26)) != 0; // SSE2

        // AVX2 je v extended leaf 7
        __cpuid(cpuInfo, 7);
        f.avx2 = (cpuInfo[1] & (1 << 5)) != 0; // AVX2

        return f;
    }



    // forward - kvuli pouziti friend
    template<typename D, typename T> class DeviceBase;

    // forward
    class DeviceDIB;
    class DeviceTargetDIB;

    // Device traits
    template<>
    struct DeviceTraits<DeviceDIB>
    {
        template<typename T>
        using GpuBuffer = std::vector<T>;

        using GpuBuffer3D = GpuBuffer<float>;
        using GpuBuffer2D = GpuBuffer<float>;
        using GpuIndexBuffer = GpuBuffer<uint32_t>;
    };

    // alias - schovame pred svetem - pouze pro interni zjednoduseni
    namespace internal {
        using DeviceDIBBase = DeviceBase<DeviceDIB, DeviceTargetDIB>;
    };

    class DeviceDIB : public internal::DeviceDIBBase
    {
        template<typename D, typename T>
        friend class DeviceBase;   // Base má pøístup do private Derived ...Impl(), ktere nemaji byt videt zvenci

    private:

        using Self = DeviceDIB;
        using Base = internal::DeviceDIBBase;

        RegistryDIB<Self> registry;


		ClearMode clearMode = ClearMode::SSE2;
		CpuFeatures cpuFeatures = detectCpuFeatures();

    public:

        /* problem s nenapovidanim IDE to nevyresilo
        // --- explicitní pøepublikování aliasù z Base pro lepší viditelnost v IDE ---
        using Context = typename Base::Context;
        using Target = typename Base::Target;
        using TargetHandle = typename Base::TargetHandle;
        using TargetRegistry = typename Base::TargetRegistry;
        static constexpr TargetHandle TARGET_INVALID = Base::TARGET_INVALID;
        // ------------------------------------------------------------------------
        */
    public:

        DeviceDIB(WindowWin32& window) 
            : Base(window) 
        {
        }

    private:

        using GpuBuffer3D = DeviceTraits<Self>::GpuBuffer3D;
        using GpuBuffer2D = DeviceTraits<Self>::GpuBuffer2D;
        using GpuIndexBuffer = DeviceTraits<Self>::GpuIndexBuffer;

        TargetHandle targetCreateImpl(const RenderTargetDescriptor& descriptor) noexcept 
        {
            return registry.targets.add(descriptor);
        }

        TargetHandle targetResizeImpl(TargetHandle target_h, uint32_t width, uint32_t height) noexcept
        {
			if (!registry.targets.isValid(target_h)) return TARGET_INVALID;
			if (width == 0 || height == 0) return TARGET_INVALID;
            
            auto descriptor = registry.targets.get(target_h).descriptor;
            descriptor.width = width;
            descriptor.height = height;
            registry.targets.reset(target_h, descriptor);
            return target_h;
		}

        Target& targetGetImpl(TargetHandle targetHandle)
        {
            if (!registry.targets.isValid(targetHandle)) throw std::runtime_error("Invalid TargetHandle: " + std::to_string(targetHandle.index) + ", " + std::to_string(targetHandle.generation));
            return registry.targets.get(targetHandle);
		}

        void drawStaticTestMeshImpl(const Context& ctx, float scaleFactor) noexcept
        {
            
            if (!registry.targets.isValid(ctx.framebufferHandle)) return;

            Target& target = registry.targets.get(ctx.framebufferHandle);

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

            Mtx4 mvp = ctx.projection * ctx.view * ctx.model * Mtx4::Scaling(scaleFactor);

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

        //void drawMeshEnqueueImpl(const Context& ctx, MeshHandle meshHandle)
        //{
        //    // @todo
        //}



        inline void clearScalar(uint32_t* dst, size_t size, uint32_t color) noexcept
        {
            std::fill_n(dst, size, color);
        }

        inline void clearSSE2(uint32_t* dst, size_t size, uint32_t color) noexcept
        {
            __m128i fill = _mm_set1_epi32(color);

            size_t i = 0;
            size_t simdCount = size / 4;

            for (size_t n = 0; n < simdCount; ++n) {
                _mm_storeu_si128((__m128i*)(dst + i), fill);
                i += 4;
            }

            for (; i < size; ++i)
                dst[i] = color;
        }

        inline void clearAVX2(uint32_t* dst, size_t size, uint32_t color) noexcept
        {
            __m256i fill = _mm256_set1_epi32(color);

            size_t i = 0;
            size_t simdCount = size / 8;

            for (size_t n = 0; n < simdCount; ++n) {
                _mm256_storeu_si256((__m256i*)(dst + i), fill);
                i += 8;
            }

            for (; i < size; ++i)
                dst[i] = color;
        }

        void clearImpl(const Context& ctx) noexcept
        {
            if (!registry.targets.isValid(ctx.framebufferHandle)) return;

            Target& target = registry.targets.get(ctx.framebufferHandle);
            uint32_t color = ctx.clearColor.toRGBA();
            size_t size = target.descriptor.width * target.descriptor.height;
            uint32_t* dst = target.framebuffer;

            switch (clearMode)
            {
            case ClearMode::AVX2:
                if (cpuFeatures.avx2) {
                    clearAVX2(dst, size, color);
                    return;
                }
                [[fallthrough]];

            case ClearMode::SSE2:
                if (cpuFeatures.sse2) {
                    clearSSE2(dst, size, color);
                    return;
                }
                [[fallthrough]];

            case ClearMode::Scalar:
            default:
                clearScalar(dst, size, color);
                return;
            }
        }

        void presentImpl(TargetHandle targetHandle) noexcept
        {
            if (!registry.targets.isValid(targetHandle)) return;

            Target& target = registry.targets.get(targetHandle);
            HDC targetDC = window.getHDC();
            HDC sourceDC = target.getDC();
            uint32_t width = target.descriptor.width;
            uint32_t height = target.descriptor.height;
            BitBlt(targetDC, 0, 0, width, height, sourceDC, 0, 0, SRCCOPY);
        }

        void registerMeshImpl(const Mesh& mesh) noexcept
        {
            
            //registry.vertexBuffer.positions.push_back(3.14f);
            //registry.vertexBuffer.positions.push_back(4.0f);
            //registry.vertexBuffer.positions.push_back(54.2f);

            /*
            std::cout << "RegisterMesh(@todo)" << std::endl;
            for (auto v : registry.vertexBuffer.positions) {

                std::cout << v << std::endl;
            }
            */
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

