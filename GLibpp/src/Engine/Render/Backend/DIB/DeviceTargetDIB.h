#pragma once

#include "DeviceTargetBase.h"
#include "RenderTargetDescriptor.h"

#include <cmath>       // sqrt (skalarni kernel konverze hloubky)
#include <cstdint>
#include <immintrin.h> // SSE2/AVX2 kernely konverze hloubky
#include <vector>

namespace GLibpp::Render {

    // ---- vizualizace hloubky: float NDC z -> grayscale ARGB ----
    // near (z = -1) -> 255 (svetla), far/clear (z = +1) -> 0 (cerna).
    // NDC z je po perspektivnim deleni silne nelinearni (vetsina sceny lezi
    // u 1.0), linearni mapovani by dalo temer cernou plochu - proto se
    // t = clamp((1 - d) * 0.5) zesvetluje gamma 1/4 (dvoji sqrt; IEEE sqrt
    // je korektne zaokrouhlena, takze skalar i SIMD daji bit-shodny vystup):
    //   g = trunc(sqrt(sqrt(t)) * 255);  pixel = 0xFF000000 | g|g<<8|g<<16
    // Tri varianty podle instrukcni sady - vyber dela volajici (DeviceDIB zna
    // cpuFeatures); SIMD bere bloky, ocasek jde skalarne. Vsechny varianty
    // schvalne truncuji (cvtTps) - bit-shodu pinuje test.
    // Free funkce (ne metody Device): testy je volaji bez okna, stejne jako
    // targety v tomto headeru.

    inline void convertDepthToGrayScalar(const float* src, uint32_t* dst, size_t count) noexcept
    {
        for (size_t i = 0; i < count; ++i)
        {
            float t = (1.0f - src[i]) * 0.5f;
            if (t < 0.0f) t = 0.0f; // clamp pred sqrt - zaporny vstup by dal NaN
            if (t > 1.0f) t = 1.0f;
            const float g = std::sqrt(std::sqrt(t)) * 255.0f;
            const uint32_t v = static_cast<uint32_t>(g);
            dst[i] = 0xFF000000u | v | (v << 8) | (v << 16);
        }
    }

    inline void convertDepthToGraySSE2(const float* src, uint32_t* dst, size_t count) noexcept
    {
        const __m128 one   = _mm_set1_ps(1.0f);
        const __m128 half  = _mm_set1_ps(0.5f);
        const __m128 zero  = _mm_setzero_ps();
        const __m128 c255  = _mm_set1_ps(255.0f);
        const __m128i alpha = _mm_set1_epi32(static_cast<int>(0xFF000000u));

        size_t i = 0;
        for (size_t n = count / 4; n > 0; --n, i += 4)
        {
            __m128 t = _mm_mul_ps(_mm_sub_ps(one, _mm_loadu_ps(src + i)), half);
            t = _mm_min_ps(_mm_max_ps(t, zero), one);
            const __m128 g = _mm_mul_ps(_mm_sqrt_ps(_mm_sqrt_ps(t)), c255);
            const __m128i v = _mm_cvttps_epi32(g); // truncation - shodne se skalarnim castem
            // v * 0x010101 pres shift+or (mullo_epi32 je az SSE4.1)
            const __m128i pix = _mm_or_si128(alpha,
                _mm_or_si128(v, _mm_or_si128(_mm_slli_epi32(v, 8), _mm_slli_epi32(v, 16))));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), pix);
        }
        convertDepthToGrayScalar(src + i, dst + i, count - i);
    }

    inline void convertDepthToGrayAVX2(const float* src, uint32_t* dst, size_t count) noexcept
    {
        const __m256 one   = _mm256_set1_ps(1.0f);
        const __m256 half  = _mm256_set1_ps(0.5f);
        const __m256 zero  = _mm256_setzero_ps();
        const __m256 c255  = _mm256_set1_ps(255.0f);
        const __m256i alpha = _mm256_set1_epi32(static_cast<int>(0xFF000000u));

        size_t i = 0;
        for (size_t n = count / 8; n > 0; --n, i += 8)
        {
            __m256 t = _mm256_mul_ps(_mm256_sub_ps(one, _mm256_loadu_ps(src + i)), half);
            t = _mm256_min_ps(_mm256_max_ps(t, zero), one);
            const __m256 g = _mm256_mul_ps(_mm256_sqrt_ps(_mm256_sqrt_ps(t)), c255);
            const __m256i v = _mm256_cvttps_epi32(g);
            const __m256i pix = _mm256_or_si256(alpha,
                _mm256_or_si256(v, _mm256_or_si256(_mm256_slli_epi32(v, 8), _mm256_slli_epi32(v, 16))));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), pix);
        }
        convertDepthToGrayScalar(src + i, dst + i, count - i);
    }

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