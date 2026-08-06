#pragma once

#include "RasterizerDIB.h"
#include "Color.h"
#include "DeviceBase.h"
#include "WindowWin32.h"
#include "Win32Dc.h"

#include <vector>
#include <algorithm>
#include <immintrin.h> // AVX2

namespace GLibpp::Render {

    // range zaregistrovaneho meshe v residency polich backendu
    // offsety urcuje VYHRADNE backend (jiny backend = jiny layout); ResourceManager zna jen identitu = handle
    struct MeshRangeDIB {
        uint32_t vertexOffset = 0;
        uint32_t vertexCount = 0;
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        uint32_t generation = 0;   // validace stale handlu
        bool valid = false;
    };

    template<typename Device>
    struct RegistryDIB
    {
        // privatni residency DIB backendu: kopie geometrie vsech meshu v jednom velkem poli
        // (stejny flow jako GL backend: upload pri startu, re-upload po mutaci; substrat pro budouci SoA/SIMD)
        // pozn.: zisk cache lokality je dnes marginalni - primarni duvod je referencni GL-shaped architektura
        std::vector<Vec4> meshPositions;
        std::vector<uint32_t> meshIndices;    // indexy lokalni vuci meshi (rasterizace jde pres scratch buffer)
        std::vector<MeshRangeDIB> meshRanges; // indexovano handle.index -> O(1) lookup bez hashovani

        typename Device::TargetRegistry targets;
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
        friend class DeviceBase;   // Base má přístup do private Derived ...Impl(), ktere nemaji byt videt zvenci

    private:

        using Self = DeviceDIB;
        using Base = internal::DeviceDIBBase;

        RegistryDIB<Self> registry;


		ClearMode clearMode = ClearMode::SSE2;
		CpuFeatures cpuFeatures = detectCpuFeatures();

        std::vector<float> floatBuffer;
        std::vector<float> viewPosBuffer; // view-space pozice (pro vypocet normal)

    public:

        /* problem s nenapovidanim IDE to nevyresilo
        // --- explicitní přepublikování aliasů z Base pro lepší viditelnost v IDE ---
        using Context = typename Base::Context;
        using Target = typename Base::Target;
        using TargetHandle = typename Base::TargetHandle;
        using TargetRegistry = typename Base::TargetRegistry;
        static constexpr TargetHandle TARGET_INVALID = Base::TARGET_INVALID;
        // ------------------------------------------------------------------------
        */
    public:

        DeviceDIB(Platform::WindowWin32& window)
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


        static float intersection(float Ax, float Ay, float Az, float Aw, float Bx, float By, float Bz, float Bw, float a, float b, float c, float d, float& px, float& py, float& pz, float& pw) {
            
            // usecka (Ax,Ay,Az) (Bx,By,Bz)
            // rovina a,b,c,d

            auto r = [a,b,c,d](float x, float y, float z, float w) {
                return a * x + b * y + c * z + d * w;
            };

            float ra = r(Ax, Ay, Az, Aw);
            float rb = r(Bx, By, Bz, Bw);
            float dr = rb - ra;
            if (fabs(dr) < 10e-6f) {
                px = py = pz = pw = 0.0f;
                return -1.0f;
            }

            float t = - ra / dr;
            
            px = Ax + t * (Bx - Ax);
            py = Ay + t * (By - Ay);
            pz = Az + t * (Bz - Az);
            pw = Aw + t * (Bw - Aw);

            return t;
        }

        inline void clipSegmentWithPlane(Vec4& A, Vec4& B, const Vec4& plane)
        {
            // Hodnoty roviny v bodech
            float ra = plane.x * A.x + plane.y * A.y + plane.z * A.z + plane.w * A.w;
            float rb = plane.x * B.x + plane.y * B.y + plane.z * B.z + plane.w * B.w;

            // Oba venku -> úsečka zmizí
            if (ra < 0 && rb < 0) {
                A = B = { 0,0,0,0 };
                return;
            }

            // A venku, B uvnitr -> posun A
            if (ra < 0 && rb >= 0) {
                float px, py, pz, pw;
                intersection(A.x, A.y, A.z, A.w, B.x, B.y, B.z, B.w, plane.x, plane.y, plane.z, plane.w, px, py, pz, pw);
                A = { px, py, pz, pw };
            }

            // B venku, A uvnitr -> posun B
            if (rb < 0 && ra >= 0) {
                float px, py, pz, pw;
                intersection(A.x, A.y, A.z, A.w, B.x, B.y, B.z, B.w, plane.x, plane.y, plane.z, plane.w, px, py, pz, pw);
                B = { px, py, pz, pw };
            }
        }



        // handle-based kresleni z vlastni residency - po uploadu je backend sobestacny,
        // na kanonicka data v ResourceManageru uz nesaha
        void drawMeshImpl(const Context& ctx, Assets::MeshHandle h, const Mtx4& transform, const Color& color, bool wiredFlag) noexcept
        {
            if (h.index >= registry.meshRanges.size()) return;
            const MeshRangeDIB& range = registry.meshRanges[h.index];
            if (!range.valid || range.generation != h.generation) return;

            rasterizeMesh(ctx,
                registry.meshPositions.data() + range.vertexOffset, range.vertexCount,
                registry.meshIndices.data() + range.indexOffset, range.indexCount,
                transform, color, wiredFlag);
        }

        // spolecne rasterizacni jadro - geometrie prichazi jako pointer + pocet
        // (bud primo kanonicky Mesh, nebo range z residency poli)
        void rasterizeMesh(const Context& ctx, const Vec4* positions, size_t vertexCount,
                           const uint32_t* indices, size_t indexCount,
                           const Mtx4& transform, const Color& color, bool wiredFlag) noexcept
        {
            if (!registry.targets.isValid(ctx.framebufferHandle)) return;

            // --- 1) Matice ---
            Mtx4 mv = ctx.getModelView(transform);
			Mtx4 mvp = ctx.getModelViewProjection(transform);

            uint32_t x = ctx.getViewport().x;
            uint32_t y = ctx.getViewport().y;
            uint32_t width = ctx.getViewport().width;
            uint32_t height = ctx.getViewport().height;

            auto viewportTransform = [&](Vec4& v) {
                v.x = (v.x * 0.5f + 0.5f) * width + x;
                v.y = (-v.y * 0.5f + 0.5f) * height + y;
                };

            // --- 2) Buffery ---
            if (floatBuffer.size() < 3 * vertexCount)
                floatBuffer.resize(3 * vertexCount);

            // view-space pozice pro výpočet normál
            if (viewPosBuffer.size() < 3 * vertexCount)
                viewPosBuffer.resize(3 * vertexCount);

            // --- 3) Transformace vrcholů ---
            int offset = 0;
            int offsetView = 0;

            for (size_t vi = 0; vi < vertexCount; ++vi)
            {
                const Vec4& vertex = positions[vi];
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

                viewPosBuffer[offsetView++] = vView.x;
                viewPosBuffer[offsetView++] = vView.y;
                viewPosBuffer[offsetView++] = vView.z;
            }

            Target& target = registry.targets.get(ctx.framebufferHandle);

            // --- 4) Směrové světlo ve VIEW SPACE ---
            float Lx = 0.0f;
            float Ly = 0.0f;
            float Lz = -1.0f; // světlo zepředu v prostoru kamery

            float lenL = std::sqrt(Lx * Lx + Ly * Ly + Lz * Lz);
            Lx /= lenL; Ly /= lenL; Lz /= lenL;

            // difuzní rozsah
            float a = 0.2f;
            float b = 1.0f;

            // --- 5) Rasterizace trojúhelníků ---
            for (size_t i = 0; i + 2 < indexCount; i += 3)
            {
                uint32_t ia = indices[i];
                uint32_t ibb = indices[i + 1];
                uint32_t ic = indices[i + 2];

                // --- view-space pozice pro normálu ---
                float axv = viewPosBuffer[3 * ia];
                float ayv = viewPosBuffer[3 * ia + 1];
                float azv = viewPosBuffer[3 * ia + 2];

                float bxv = viewPosBuffer[3 * ibb];
                float byv = viewPosBuffer[3 * ibb + 1];
                float bzv = viewPosBuffer[3 * ibb + 2];

                float cxv = viewPosBuffer[3 * ic];
                float cyv = viewPosBuffer[3 * ic + 1];
                float czv = viewPosBuffer[3 * ic + 2];

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
                RasterizerDIB::drawTriangle(
                    target,
                    ax, ay,
                    bx, by,
                    cx, cy,
                    shaded,
                    wiredFlag
                );
            }
        }



        void drawAxisImpl(const Context& ctx, const Mtx4& transform)
        {
            if (!registry.targets.isValid(ctx.framebufferHandle))
                return;

            // MVP
            Mtx4 mv = ctx.getModelView(transform);
            Mtx4 mvp = ctx.getModelViewProjection(transform);

            uint32_t x = ctx.getViewport().x;
            uint32_t y = ctx.getViewport().y;
            uint32_t width = ctx.getViewport().width;
            uint32_t height = ctx.getViewport().height;

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
            // 1) MVP transform
            for (int i = 0; i < 6; i++) 
            {
                axisVerts[i] = mvp * axisVerts[i];
            }

            // 2) Clipping (jen jednou!)
            Vec4 planeX = { 1.0f, 0.0f, 0.0f, 0.5f };
            clipSegmentWithPlane(axisVerts[0], axisVerts[1], planeX);
            clipSegmentWithPlane(axisVerts[2], axisVerts[3], planeX);
            clipSegmentWithPlane(axisVerts[4], axisVerts[5], planeX);

            // 3) Divide W
            for (int i = 0; i < 6; i++)
            {
                axisVerts[i].divideW();
            }
            
            // 4) Viewport transform
            for (int i = 0; i < 6; i++)
            {
                viewportTransform(axisVerts[i]);
            }
                
            Target& target = registry.targets.get(ctx.framebufferHandle);

            // X axis (red)
            RasterizerDIB::drawLine(
                target,
                (int)axisVerts[0].x, (int)axisVerts[0].y,
                (int)axisVerts[1].x, (int)axisVerts[1].y,
                0xFFFF0000
            );

            // Y axis (green)
            RasterizerDIB::drawLine(
                target,
                (int)axisVerts[2].x, (int)axisVerts[2].y,
                (int)axisVerts[3].x, (int)axisVerts[3].y,
                0xFF00FF00
            );

            // Z axis (blue)
            RasterizerDIB::drawLine(
                target,
                (int)axisVerts[4].x, (int)axisVerts[4].y,
                (int)axisVerts[5].x, (int)axisVerts[5].y,
                0xFF0000FF
            );
        }


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

        // upload geometrie do residency poli - vola se z upload walku na zacatku runLoop
        void meshRegisterImpl(Assets::MeshHandle h, const Geometry::Mesh& mesh) noexcept
        {
            const auto& vb = mesh.getVertexBuffer();
            const auto& ib = mesh.getIndexBuffer();

            if (registry.meshRanges.size() <= h.index)
                registry.meshRanges.resize(h.index + 1);

            MeshRangeDIB range;
            range.vertexOffset = static_cast<uint32_t>(registry.meshPositions.size());
            range.vertexCount = static_cast<uint32_t>(vb.size());
            range.indexOffset = static_cast<uint32_t>(registry.meshIndices.size());
            range.indexCount = static_cast<uint32_t>(ib.size());
            range.generation = h.generation;
            range.valid = true;

            registry.meshPositions.insert(registry.meshPositions.end(), vb.begin(), vb.end());
            registry.meshIndices.insert(registry.meshIndices.end(), ib.begin(), ib.end());
            registry.meshRanges[h.index] = range;
        }

        // re-upload dat po mutaci kanonickeho meshe (dynamicke meshe - GridWave)
        // velikosti se menit nesmi - range je v polich pevne dany
        void meshUpdateImpl(Assets::MeshHandle h, const Geometry::Mesh& mesh) noexcept
        {
            if (h.index >= registry.meshRanges.size()) return;
            const MeshRangeDIB& range = registry.meshRanges[h.index];
            if (!range.valid || range.generation != h.generation) return;

            const auto& vb = mesh.getVertexBuffer();
            const auto& ib = mesh.getIndexBuffer();
            if (vb.size() != range.vertexCount || ib.size() != range.indexCount) return;

            std::copy(vb.begin(), vb.end(), registry.meshPositions.begin() + range.vertexOffset);
            std::copy(ib.begin(), ib.end(), registry.meshIndices.begin() + range.indexOffset);
        }

        Context createContextImpl() noexcept {
            Context ctx;
            return ctx;
        }

    };

}

