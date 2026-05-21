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

        std::vector<float> floatBuffer;

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

        void drawMeshImpl(const Context& ctx, const Mesh& mesh, const Mtx4& transform, const Color& color, bool wiredFlag) noexcept
        {
            if (!registry.targets.isValid(ctx.framebufferHandle)) return;

            // --- 1) Matice ---
            Mtx4 mv = ctx.view * transform;          // model-view
            Mtx4 mvp = ctx.projection * mv;           // model-view-projection

            uint32_t x = ctx.viewport.x;
            uint32_t y = ctx.viewport.y;
            uint32_t width = ctx.viewport.width;
            uint32_t height = ctx.viewport.height;

            auto viewportTransform = [&](Vec4& v) {
                v.x = (v.x * 0.5f + 0.5f) * width + x;
                v.y = (-v.y * 0.5f + 0.5f) * height + y;
                };

            // --- 2) Buffery ---
            size_t vertexCount = mesh.getVertexBuffer().size();

            if (floatBuffer.size() < 3 * vertexCount)
                floatBuffer.resize(3 * vertexCount);

            // view-space pozice pro výpoèet normál
            static std::vector<float> viewPos;
            if (viewPos.size() < 3 * vertexCount)
                viewPos.resize(3 * vertexCount);

            // --- 3) Transformace vrcholù ---
            int offset = 0;
            int offsetView = 0;

            for (const auto& vertex : mesh.getVertexBuffer())
            {
                // projekce + viewport
                Vec4 v = mvp * vertex;
                if (fabs(v.w) > 10e-6) 
                {
                    
                    bool inside =
                        v.x >= -v.w && v.x <= v.w &&
                        v.y >= -v.w && v.y <= v.w &&
                        v.z >= -v.w && v.z <= v.w;

                    if (inside) {
                        v.divideW();
                        viewportTransform(v);
                    }
                    else {
                        v.z = 0.0f;
                    }
                    
                }
                else 
                {
                    //v = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
                }

                floatBuffer[offset++] = v.x;
                floatBuffer[offset++] = v.y;
                floatBuffer[offset++] = v.z;

                // view-space pozice (pro normály)
                Vec4 vView = mv * vertex;
                vView.divideW();

                viewPos[offsetView++] = vView.x;
                viewPos[offsetView++] = vView.y;
                viewPos[offsetView++] = vView.z;
            }

            Target& target = registry.targets.get(ctx.framebufferHandle);

            // --- 4) Smìrové svìtlo ve VIEW SPACE ---
            float Lx = 0.0f;
            float Ly = 0.0f;
            float Lz = -1.0f; // svìtlo zepøedu v prostoru kamery

            float lenL = std::sqrt(Lx * Lx + Ly * Ly + Lz * Lz);
            Lx /= lenL; Ly /= lenL; Lz /= lenL;

            // difuzní rozsah
            float a = 0.2f;
            float b = 1.0f;

            // --- 5) Rasterizace trojúhelníkù ---
            const auto& ib = mesh.getIndexBuffer();

            for (int i = 0; i < ib.size(); i += 3)
            {
                int ia = ib[i];
                int ibb = ib[i + 1];
                int ic = ib[i + 2];

                // --- view-space pozice pro normálu ---
                float axv = viewPos[3 * ia];
                float ayv = viewPos[3 * ia + 1];
                float azv = viewPos[3 * ia + 2];

                float bxv = viewPos[3 * ibb];
                float byv = viewPos[3 * ibb + 1];
                float bzv = viewPos[3 * ibb + 2];

                float cxv = viewPos[3 * ic];
                float cyv = viewPos[3 * ic + 1];
                float czv = viewPos[3 * ic + 2];

                // --- normála ve view space ---
                float ABx = bxv - axv;
                float ABy = byv - ayv;
                float ABz = bzv - azv;

                float ACx = cxv - axv;
                float ACy = cyv - ayv;
                float ACz = czv - azv;

                float Nx = ABy * ACz - ABz * ACy;
                float Ny = ABz * ACx - ABx * ACz;
                float Nz = ABx * ACy - ABy * ACx;

                float lenN = std::sqrt(Nx * Nx + Ny * Ny + Nz * Nz);
                if (lenN > 0.00001f) {
                    Nx /= lenN; Ny /= lenN; Nz /= lenN;
                }

                // --- Lambert ---
                float dotNL = Nx * Lx + Ny * Ly + Nz * Lz;
                if (dotNL < 0.0f) dotNL = 0.0f;
                if (dotNL > 1.0f) dotNL = 1.0f;

                float I = a + dotNL * (b - a);

                // --- barva ---
                uint32_t shaded = Color(
                    uint8_t(color.r * I),
                    uint8_t(color.g * I),
                    uint8_t(color.b * I),
                    color.a
                ).toRGBA();

                // --- screen-space pozice ---
                float ax = floatBuffer[3 * ia];
                float ay = floatBuffer[3 * ia + 1];
                float az = floatBuffer[3 * ia + 2];

                float bx = floatBuffer[3 * ibb];
                float by = floatBuffer[3 * ibb + 1];
                float bz = floatBuffer[3 * ibb + 2];

                float cx = floatBuffer[3 * ic];
                float cy = floatBuffer[3 * ic + 1];
                float cz = floatBuffer[3 * ic + 2];

                bool skip = fabs(az) < 10e-6 || fabs(bz) < 10e-6 || fabs(cz) < 10e-6;

                if (!skip)
                RasterizatorDIB::drawTriangle(
                    target,
                    ax, ay,
                    bx, by,
                    cx, cy,
                    shaded,
                    wiredFlag
                );
            }
        }



        void drawMeshImpl2(const Context& ctx, const Mesh& mesh, const Mtx4& transform, const Color& color) noexcept
        {
            if (!registry.targets.isValid(ctx.framebufferHandle)) return;

            Mtx4 mvp = ctx.projection * ctx.view * transform;

            uint32_t x = ctx.viewport.x;
            uint32_t y = ctx.viewport.y;
            uint32_t width = ctx.viewport.width;
            uint32_t height = ctx.viewport.height;

            auto viewportTransform = [&](Vec4& v) {
                v.x = (v.x * 0.5f + 0.5f) * width + x;
                v.y = (-v.y * 0.5f + 0.5f) * height + y;
            };
            
            auto vertexCount = mesh.getVertexBuffer().size();
            if (floatBuffer.size() < 3*vertexCount)
            {
                floatBuffer.resize(3*vertexCount);
            }

            for (int offset = 0; const auto& vertex : mesh.getVertexBuffer())
            {
                auto vertex_ = mvp * vertex;
                vertex_.divideW();

                viewportTransform(vertex_);

                floatBuffer[offset++] = vertex_.x;
                floatBuffer[offset++] = vertex_.y;
                floatBuffer[offset++] = vertex_.z;
            }

            Target& target = registry.targets.get(ctx.framebufferHandle);

            for (int i = 0; i < mesh.getIndexBuffer().size()/3; i++)
            {
                int ia = mesh.getIndexBuffer()[3*i];
                int ib = mesh.getIndexBuffer()[3*i + 1];
                int ic = mesh.getIndexBuffer()[3*i + 2];

                float vax = floatBuffer[3*ia];
                float vay = floatBuffer[3*ia + 1];

                float vbx = floatBuffer[3*ib];
                float vby = floatBuffer[3*ib + 1];

                float vcx = floatBuffer[3*ic];
                float vcy = floatBuffer[3*ic + 1];

                RasterizatorDIB::drawTriangle(
                    target,
                    vax, vay,
                    vbx, vby,
                    vcx, vcy,
                    color.toRGBA(),
                    false
                );
            }
        }

        void drawAxisImpl(const Context& ctx, const Mtx4& transform)
        {
            if (!registry.targets.isValid(ctx.framebufferHandle))
                return;

            // MVP
            Mtx4 mvp = ctx.projection * ctx.view * transform;

            // Viewport
            uint32_t x = ctx.viewport.x;
            uint32_t y = ctx.viewport.y;
            uint32_t width = ctx.viewport.width;
            uint32_t height = ctx.viewport.height;

            auto viewportTransform = [&](Vec4& v) {
                v.x = (v.x * 0.5f + 0.5f) * width + x;
                v.y = (-v.y * 0.5f + 0.5f) * height + y;
                };

            // 3 osy: každá má dva body
            Vec4 axisVerts[6] = {
                // X axis
                {0,0,0,1}, {1,0,0,1},
                // Y axis
                {0,0,0,1}, {0,1,0,1},
                // Z axis
                {0,0,0,1}, {0,0,1,1}
            };

            // Transformace + viewport
            for (int i = 0; i < 6; i++)
            {
                axisVerts[i] = mvp * axisVerts[i];
                axisVerts[i].divideW();
                viewportTransform(axisVerts[i]);
            }

            Target& target = registry.targets.get(ctx.framebufferHandle);

            // X axis (red)
            RasterizatorDIB::drawLine(
                target,
                (int)axisVerts[0].x, (int)axisVerts[0].y,
                (int)axisVerts[1].x, (int)axisVerts[1].y,
                0xFFFF0000
            );

            // Y axis (green)
            RasterizatorDIB::drawLine(
                target,
                (int)axisVerts[2].x, (int)axisVerts[2].y,
                (int)axisVerts[3].x, (int)axisVerts[3].y,
                0xFF00FF00
            );

            // Z axis (blue)
            RasterizatorDIB::drawLine(
                target,
                (int)axisVerts[4].x, (int)axisVerts[4].y,
                (int)axisVerts[5].x, (int)axisVerts[5].y,
                0xFF0000FF
            );
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
                Color::Grayscale(0.5f).toRGBA(),
                false
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

