#pragma once

#include "RasterizerDIB.h"
#include "Color.h"
#include "DeviceBase.h"
#include "WindowWin32.h"
#include "Win32Dc.h"

#include <vector>
#include <algorithm>
#include <bit>
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
        std::vector<uint8_t> vertexValidBuffer; // 1 = vrchol cely uvnitr frustumu

        // Hloubka se cisti na nejvzdalenejsi hodnotu. Po perspektivnim deleni je
        // NDC z v [-1, 1] (GL konvence, -1 = near rovina), takze far = +1
        // a depth test je "mensi vyhrava" (viz RasterizerDIB::drawTriangle).
        static constexpr float kDepthFar = 1.0f;

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


        // orizne homogenni usecku AB rovinou v clip space (uvnitr = dot(plane, P) >= 0);
        // vraci false, kdyz je usecka cela venku - volajici ji pak nesmi kreslit.
        // Deleni je bezpecne: do vypoctu t se jde jen kdyz maji ra/rb opacna znamenka,
        // takze jmenovatel neni nikdy nula (zadna NaN cesta jako u drivejsi verze,
        // ktera pri "cela venku" vracela degenerovany bod {0,0,0,0}).
        static bool clipSegmentWithPlane(Vec4& A, Vec4& B, const Vec4& plane) noexcept
        {
            float ra = plane.x * A.x + plane.y * A.y + plane.z * A.z + plane.w * A.w;
            float rb = plane.x * B.x + plane.y * B.y + plane.z * B.z + plane.w * B.w;

            if (ra < 0.0f && rb < 0.0f) return false; // cela venku
            if (ra >= 0.0f && rb >= 0.0f) return true; // cela uvnitr

            float t = ra / (ra - rb); // parametr pruseciku s rovinou
            Vec4 P = A + (B - A) * t;
            if (ra < 0.0f) A = P; else B = P;
            return true;
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

            if (vertexValidBuffer.size() < vertexCount)
                vertexValidBuffer.resize(vertexCount);

            // --- 3) Transformace vrcholů ---
            int offset = 0;
            int offsetView = 0;

            for (size_t vi = 0; vi < vertexCount; ++vi)
            {
                const Vec4& vertex = positions[vi];
                // projekce + viewport
                Vec4 v = mvp * vertex;

                // Vrchol je platny jen kdyz je cely uvnitr frustumu (chybi clipping
                // trojuhelniku - trojuhelnik s neplatnym vrcholem se zahodi).
                // Priznak je vlastni pole: drive se neplatnost signalizovala pres
                // v.z = 0, coz kolidovalo s legitimni NDC hloubkou 0 - a depth
                // buffer takovy preteceny sentinel neunese vubec.
                const bool valid = fabs(v.w) > 1e-5f
                    && v.x >= -v.w && v.x <= v.w
                    && v.y >= -v.w && v.y <= v.w
                    && v.z >= -v.w && v.z <= v.w;

                if (valid) {
                    v.divideW();
                    viewportTransform(v);
                }

                vertexValidBuffer[vi] = valid ? uint8_t(1) : uint8_t(0);

                floatBuffer[offset++] = v.x;
                floatBuffer[offset++] = v.y;
                floatBuffer[offset++] = v.z; // NDC z v [-1, 1] pro platne vrcholy

                // view-space pozice (pro normály)
                Vec4 vView = mv * vertex;
                vView.divideW();

                viewPosBuffer[offsetView++] = vView.x;
                viewPosBuffer[offsetView++] = vView.y;
                viewPosBuffer[offsetView++] = vView.z;
            }

            Target& target = registry.targets.get(ctx.framebufferHandle);

            // depth target je volitelny: kdyz neni bindnuty (nebo nesedi rozliseni
            // s framebufferem), kresli se jako dosud bez Z-testu v poradi commandu
            Target* depth = nullptr;
            if (registry.targets.isValid(ctx.depthbufferHandle))
            {
                Target& d = registry.targets.get(ctx.depthbufferHandle);
                if (d.isDepthUsable() && d.pixelCount() == target.pixelCount()
                    && d.descriptor.width == target.descriptor.width)
                {
                    depth = &d;
                }
            }

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

                // ochrana proti vadnym indexum (napr. z pokazeneho .obj):
                // scratch buffery jen rostou, takze index mimo rozsah by cetl
                // stale hodnoty drive kresleneho meshe = ticha vizualni korupce
                if (ia >= vertexCount || ibb >= vertexCount || ic >= vertexCount)
                    continue;

                // chybi clipping trojuhelniku - trojuhelnik trcici z frustumu se cely
                // zahodi (viditelne mizeni na okrajich; drive resil sentinel z == 0)
                if (!vertexValidBuffer[ia] || !vertexValidBuffer[ibb] || !vertexValidBuffer[ic])
                    continue;

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

                RasterizerDIB::drawTriangle(
                    target, depth,
                    ax, ay, az,
                    bx, by, bz,
                    cx, cy, cz,
                    shaded,
                    wiredFlag
                );
            }
        }



        void drawAxisImpl(const Context& ctx, const Mtx4& transform)
        {
            if (!registry.targets.isValid(ctx.framebufferHandle))
                return;

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

            // 1) MVP transform
            for (int i = 0; i < 6; i++)
            {
                axisVerts[i] = mvp * axisVerts[i];
            }

            // 2) Clipping proti vsem 6 rovinam frustumu v clip space (GL konvence
            //    -w <= x,y,z <= w; stejny test jako per-vertex "inside" v rasterizeMesh).
            //    Dela dve veci najednou:
            //    - near rovina {0,0,1,1} zaruci w >= nearZ > 0 pro vse, co prezije
            //      -> divideW nikdy nedeli nulou (drivejsi cesta k NaN -> (int)NaN = UB)
            //    - x/y roviny omezi NDC na [-1,1] -> Bresenham dostane souradnice
            //      uvnitr viewportu (drive dostaval garbage a stalloval na obrich usecich)
            const Vec4 frustumPlanes[6] = {
                { 1.0f,  0.0f,  0.0f, 1.0f },  // x >= -w (leva)
                {-1.0f,  0.0f,  0.0f, 1.0f },  // x <=  w (prava)
                { 0.0f,  1.0f,  0.0f, 1.0f },  // y >= -w (spodni)
                { 0.0f, -1.0f,  0.0f, 1.0f },  // y <=  w (horni)
                { 0.0f,  0.0f,  1.0f, 1.0f },  // z >= -w (near)
                { 0.0f,  0.0f, -1.0f, 1.0f },  // z <=  w (far)
            };

            Target& target = registry.targets.get(ctx.framebufferHandle);

            const uint32_t axisColors[3] = { 0xFFFF0000, 0xFF00FF00, 0xFF0000FF }; // X, Y, Z

            for (int seg = 0; seg < 3; ++seg)
            {
                Vec4 A = axisVerts[2 * seg];
                Vec4 B = axisVerts[2 * seg + 1];

                bool visible = true;
                for (const Vec4& plane : frustumPlanes)
                {
                    if (!clipSegmentWithPlane(A, B, plane)) { visible = false; break; }
                }
                if (!visible) continue;

                // 3) Divide W + viewport - po oriznuti je w >= nearZ a NDC v [-1,1]
                A.divideW();
                B.divideW();
                viewportTransform(A);
                viewportTransform(B);

                RasterizerDIB::drawLine(
                    target,
                    (int)A.x, (int)A.y,
                    (int)B.x, (int)B.y,
                    axisColors[seg]
                );
            }
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
            // hloubka (kdyz je bindnuta) - vsechny pixely na far.
            // Jede pres tentyz SIMD fill jako barva: je to jen vypln 32bitovym
            // vzorem, takze se posila bitova reprezentace floatu. V Debugu je to
            // podstatne - std::fill_n nad pul milionem floatu je tam skalarni
            // smycka a stalo to ~10 FPS na 1% Low.
            if (registry.targets.isValid(ctx.depthbufferHandle))
            {
                Target& d = registry.targets.get(ctx.depthbufferHandle);
                if (d.isDepthUsable())
                {
                    clearFill(reinterpret_cast<uint32_t*>(d.depthbuffer.data()),
                        d.depthbuffer.size(), std::bit_cast<uint32_t>(kDepthFar));
                }
            }

            if (!registry.targets.isValid(ctx.framebufferHandle)) return;

            Target& target = registry.targets.get(ctx.framebufferHandle);
            clearFill(target.framebuffer, target.pixelCount(), ctx.clearColor.toRGBA());
        }

        // vypln souvisleho pole 32bitovym vzorem podle zvoleneho clearMode
        // (barva i hloubka - hloubka posila bitovou reprezentaci floatu)
        void clearFill(uint32_t* dst, size_t size, uint32_t pattern) noexcept
        {
            switch (clearMode)
            {
            case ClearMode::AVX2:
                if (cpuFeatures.avx2) {
                    clearAVX2(dst, size, pattern);
                    return;
                }
                [[fallthrough]];

            case ClearMode::SSE2:
                if (cpuFeatures.sse2) {
                    clearSSE2(dst, size, pattern);
                    return;
                }
                [[fallthrough]];

            case ClearMode::Scalar:
            default:
                clearScalar(dst, size, pattern);
                return;
            }
        }

        void presentImpl(TargetHandle targetHandle) noexcept
        {
            if (!registry.targets.isValid(targetHandle)) return;

            Target& target = registry.targets.get(targetHandle);
            if (target.isDepth()) return; // depth target nema DIB sekci ani DC, neni co blitovat

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

