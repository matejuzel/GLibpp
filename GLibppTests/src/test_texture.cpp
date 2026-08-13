// Textury: GDI+ loader (ImageLoaderWin32), registrace v ResourceManageru,
// ShaderTextured (nearest sampling, wrap, fallback bez textury) a perspektivne
// korektni interpolace UV v rasterizeru.

#define NOMINMAX          // ImageLoaderWin32.h je na NOMINMAX pripraveny (Gdiplus trik)
#include <windows.h>
#include <intrin.h>       // __cpuid (guard AVX2 vetve testu)

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "RenderTargetDescriptor.h"
#include "RasterizerDIB.h"
#include "RipMapDIB.h"
#include "ImageLoaderWin32.h"
#include "ResourceManager.h"

#include "TestRunner.h"

using GLibpp::Assets::ResourceManager;
using GLibpp::Assets::TextureData;
using GLibpp::Platform::ImageLoaderWin32;
using GLibpp::Render::Color;
using GLibpp::Render::DeviceTargetDIB;
using GLibpp::Render::FragmentShaderId;
using GLibpp::Render::RasterizerDIB;
using GLibpp::Render::RenderTargetDescriptor;
using GLibpp::Render::ShaderUniforms;
using GLibpp::Render::TriangleInput;

namespace {

    constexpr int W = 16;
    constexpr int H = 16;

    // 2x2 BMP (24bpp, bez komprese) se znamymi barvami - self-contained vstup
    // pro GDI+ loader. BMP je bottom-up: prvni radek dat = SPODNI radek obrazku.
    std::string writeTestBmp()
    {
        char tempDir[MAX_PATH];
        GetTempPathA(MAX_PATH, tempDir);
        const std::string path = std::string(tempDir) + "glibpp_test_texture.bmp";

        // pixely (B, G, R), radky zdola nahoru, kazdy radek padovany na 4 bajty
        // obrazkove poradi:  (0,0) cervena  (1,0) zelena
        //                    (0,1) modra    (1,1) bila
        const uint8_t pixelData[] = {
            0xFF, 0x00, 0x00,  0xFF, 0xFF, 0xFF,  0x00, 0x00, // spodni radek: modra, bila + pad
            0x00, 0x00, 0xFF,  0x00, 0xFF, 0x00,  0x00, 0x00, // horni radek: cervena, zelena + pad
        };

        const uint32_t dataSize = sizeof(pixelData);
        const uint32_t fileSize = 54 + dataSize;

        uint8_t header[54] = {};
        header[0] = 'B'; header[1] = 'M';
        *reinterpret_cast<uint32_t*>(header + 2) = fileSize;
        *reinterpret_cast<uint32_t*>(header + 10) = 54;        // offset dat
        *reinterpret_cast<uint32_t*>(header + 14) = 40;        // BITMAPINFOHEADER
        *reinterpret_cast<int32_t*>(header + 18) = 2;          // sirka
        *reinterpret_cast<int32_t*>(header + 22) = 2;          // vyska (bottom-up)
        *reinterpret_cast<uint16_t*>(header + 26) = 1;         // planes
        *reinterpret_cast<uint16_t*>(header + 28) = 24;        // bpp
        *reinterpret_cast<uint32_t*>(header + 34) = dataSize;

        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(header), sizeof(header));
        f.write(reinterpret_cast<const char*>(pixelData), sizeof(pixelData));
        f.close();

        return path;
    }

    void clearColor(DeviceTargetDIB& t, uint32_t c)
    {
        std::fill_n(t.framebuffer, size_t(W) * H, c);
    }

    uint32_t pixelAt(const DeviceTargetDIB& t, int x, int y)
    {
        return t.framebuffer[size_t(y) * W + x];
    }

    // trojuhelniky pokryvajici ctverec [0,8)x[0,8) s UV [0,1]x[0,1] (afinni, invW = 1)
    TriangleInput quadTriA(const Color& color)
    {
        return TriangleInput{
            0.0f, 0.0f, 0.0f,   8.0f, 0.0f, 0.0f,   8.0f, 8.0f, 0.0f,
            0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,
            color,
            false
        };
    }

    TriangleInput quadTriB(const Color& color)
    {
        return TriangleInput{
            0.0f, 0.0f, 0.0f,   8.0f, 8.0f, 0.0f,   0.0f, 8.0f, 0.0f,
            0.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f,
            color,
            false
        };
    }

    // Tentyz ctverec [0,8)x[0,8), ale s volitelnym rozsahem UV: uSpan texturovych
    // opakovani na 8 pixelu siroky trojuhelnik znamena stopu uSpan * texWidth / 8
    // texelu na pixel - tim se rizeni vyberu filtracni urovne da nastavit presne.
    TriangleInput quadTriSpan(const Color& color, float uSpan, float vSpan)
    {
        return TriangleInput{
            0.0f, 0.0f, 0.0f,   8.0f, 0.0f, 0.0f,   8.0f, 8.0f, 0.0f,
            0.0f,  0.0f,  1.0f,
            uSpan, 0.0f,  1.0f,
            uSpan, vSpan, 1.0f,
            0.0f, 0.0f, 0.0f,
            color,
            false
        };
    }

}

namespace GLibppTests {

    void runFilteringTests(); // definice nize - vola se z runTextureTests

    void runTextureTests()
    {
        section("Textury - GDI+ loader");

        const std::string bmpPath = writeTestBmp();

        try
        {
            const TextureData tex = ImageLoaderWin32::Load(bmpPath);

            check(tex.isValid(), "nactena textura je validni");
            check(tex.width == 2 && tex.height == 2,
                "rozmery sedi (2x2, ma " + std::to_string(tex.width) + "x" + std::to_string(tex.height) + ")");

            if (tex.isValid() && tex.width == 2 && tex.height == 2)
            {
                // radky shora dolu, ARGB - overuje i preklopeni bottom-up BMP
                check(tex.pixels[0] == Color(255, 0, 0, 255).toRGBA(), "texel (0,0) je cerveny");
                check(tex.pixels[1] == Color(0, 255, 0, 255).toRGBA(), "texel (1,0) je zeleny");
                check(tex.pixels[2] == Color(0, 0, 255, 255).toRGBA(), "texel (0,1) je modry");
                check(tex.pixels[3] == Color(255, 255, 255, 255).toRGBA(), "texel (1,1) je bily");
            }

            // registrace v ResourceManageru
            ResourceManager rm;
            const auto h = rm.textureRegister(ImageLoaderWin32::Load(bmpPath));
            check(rm.textureIsValid(h), "textura zaregistrovana v ResourceManageru");
            check(rm.textureGet(h).width == 2, "textureGet vraci nactena data");

            bool threw = false;
            try { ImageLoaderWin32::Load("neexistujici_soubor.jpg"); }
            catch (const std::exception&) { threw = true; }
            check(threw, "neexistujici soubor hlasi vyjimku");
        }
        catch (const std::exception& e)
        {
            check(false, std::string("vyjimka: ") + e.what());
        }

        DeleteFileA(bmpPath.c_str());

        section("Textury - ShaderTextured (sampling + perspektivni korekce)");

        try
        {
            DeviceTargetDIB target(RenderTargetDescriptor::FramebufferRGBA32bit(W, H));

            // 2x2 textura primo v pameti: R G / B W (radky shora)
            const uint32_t texels[4] = {
                Color(255, 0, 0, 255).toRGBA(), Color(0, 255, 0, 255).toRGBA(),
                Color(0, 0, 255, 255).toRGBA(), Color(255, 255, 255, 255).toRGBA(),
            };
            // jednourovnova textura: jediny nulovy offset (viz invariant
            // u ShaderUniforms) - staci pro nearest i bilinearni vzorkovani
            static constexpr uint32_t kLevel0[1] = { 0u };

            const ShaderUniforms uni{ 1.0f / float(W), 1.0f / float(H), texels, 2, 2, kLevel0, 1, 1 };
            const ShaderUniforms noTex{ 1.0f / float(W), 1.0f / float(H), nullptr, 0, 0, kLevel0, 1, 1 };

            const Color base(10, 200, 60, 255);
            const auto texturedFn = RasterizerDIB::fragmentFunction(FragmentShaderId::Textured);

            // afinni pripad (invW = 1): ctverec [0,8) s UV [0,1] -> kvadranty = texely
            clearColor(target, 0u);
            texturedFn(target, nullptr, quadTriA(base), uni);
            texturedFn(target, nullptr, quadTriB(base), uni);

            check(pixelAt(target, 1, 1) == texels[0], "kvadrant (1,1) sampluje texel (0,0) - cerveny");
            check(pixelAt(target, 6, 1) == texels[1], "kvadrant (6,1) sampluje texel (1,0) - zeleny");
            check(pixelAt(target, 1, 6) == texels[2], "kvadrant (1,6) sampluje texel (0,1) - modry");
            check(pixelAt(target, 6, 6) == texels[3], "kvadrant (6,6) sampluje texel (1,1) - bily");

            // fallback bez bindnute textury = zakladni barva instance
            clearColor(target, 0u);
            texturedFn(target, nullptr, quadTriA(base), noTex);
            check(pixelAt(target, 5, 2) == base.toRGBA(), "bez textury vraci zakladni barvu instance");

            // perspektivni korekce: nerovnomerne invW musi posunout hranici texelu
            // oproti afinnimu pripadu (jinak by interpolace nebyla korektni)
            TriangleInput perspA = quadTriA(base);
            TriangleInput perspB = quadTriB(base);
            // pravy okraj "dal" od kamery: invW 1 -> 0.2 (w 1 -> 5)
            perspA.invW1 = 0.2f; perspA.invW2 = 0.2f;
            perspB.invW1 = 0.2f;

            clearColor(target, 0u);
            texturedFn(target, nullptr, perspA, uni);
            texturedFn(target, nullptr, perspB, uni);

            // afinne by pixel x=5 (stred 5.5 -> u ~ 0.69) sampoval pravy sloupec;
            // perspektivne se hranice u = 0.5 posune DOPRAVA (vzdalena cast
            // textury se mackne ke vzdalenemu okraji), takze x=5 zustava
            // v levem (cervenem) sloupci: u = (0.6875*0.2)/(0.3125 + 0.6875*0.2) ~ 0.31
            check(pixelAt(target, 5, 1) == texels[0],
                "perspektivni korekce posouva hranici texelu (afinni by tu uz byl zeleny)");
            // u praveho okraje uz u prekroci 0.5: stred 7.5 -> u = 0.75 -> pravy sloupec
            check(pixelAt(target, 7, 1) == texels[1],
                "vzdaleny okraj dosahne na pravy sloupec textury (u -> 1)");

            section("Textury - render-to-texture (ShaderResource target jako framebuffer)");

            // podstata render-to-texture na DIB: framebuffer ShaderResource targetu
            // ukazuje do texelStorage, takze rasterizer do nej umi kreslit a dalsi
            // draw ho ihned sampluje bez jakehokoli kopirovani
            DeviceTargetDIB rt(RenderTargetDescriptor::Texture(W, H));
            check(rt.isShaderResource() && rt.framebuffer != nullptr,
                "ShaderResource target ma zapisovatelny framebuffer v pameti");

            // 1) kresleni DO textury: modre pozadi, Solid vyplni ctverec [0,8) cervene
            const Color red(255, 0, 0, 255);
            const Color blue(0, 0, 255, 255);
            const auto solidFn = RasterizerDIB::fragmentFunction(FragmentShaderId::Solid);
            clearColor(rt, blue.toRGBA());
            solidFn(rt, nullptr, quadTriA(red), noTex);
            solidFn(rt, nullptr, quadTriB(red), noTex);
            check(pixelAt(rt, 3, 3) == red.toRGBA(), "rasterizer zapisuje do ShaderResource targetu");

            // 2) sampling Z prave nakreslene textury v nasledujicim drawu:
            // quad [0,8) s UV [0,1] mapuje CELOU 16x16 texturu -> pixel (2,2)
            // sampluje texel (5,5) z cerveneho ctverce, pixel (6,6) texel (13,13)
            // z modreho pozadi
            static constexpr uint32_t kRtLevel0[1] = { 0u };
            const ShaderUniforms rtUni{ 1.0f / float(W), 1.0f / float(H), rt.framebuffer,
                                        uint32_t(W), uint32_t(H), kRtLevel0, 1, 1 };
            clearColor(target, 0u);
            texturedFn(target, nullptr, quadTriA(base), rtUni);
            texturedFn(target, nullptr, quadTriB(base), rtUni);
            check(pixelAt(target, 2, 2) == red.toRGBA(), "sampling vraci obsah nakresleny do textury");
            check(pixelAt(target, 6, 6) == blue.toRGBA(), "sampling vraci i pozadi RT textury");
        }
        catch (const std::exception& e)
        {
            check(false, std::string("vyjimka pri tvorbe targetu: ") + e.what());
        }

        section("Textury - vizualizace hloubky (convertDepthToGray, SIMD)");

        // 11 hodnot - schvalne nedelitelne 4 ani 8, SIMD musi projit i ocaskem;
        // mapovani: t = clamp((1 - d) * 0.5), g = trunc(sqrt(sqrt(t)) * 255)
        // (gamma 1/4 - NDC z je nelinearni, linearni skala by byla temer cerna)
        const float depthVals[11] = { -1.0f, 0.0f, 1.0f, -2.0f, 2.0f, 0.5f, -0.5f, 0.25f, 0.75f, 0.9f, -0.9f };
        uint32_t outScalar[11] = {};
        GLibpp::Render::convertDepthToGrayScalar(depthVals, outScalar, 11);

        check(outScalar[0] == 0xFFFFFFFFu, "near (-1) -> bila");
        check(outScalar[1] == 0xFFD6D6D6u, "stred (0) -> 214 (0.5^0.25)");
        check(outScalar[2] == 0xFF000000u, "far (+1) -> cerna");
        check(outScalar[3] == 0xFFFFFFFFu, "hodnota pod -1 se clampuje na bilou");
        check(outScalar[4] == 0xFF000000u, "hodnota nad +1 se clampuje na cernou (ne NaN)");
        check(outScalar[5] == 0xFFB4B4B4u, "d = 0.5 -> 180 (kanaly shodne, alfa FF)");

        // SIMD varianty musi byt bit-shodne se skalarem (obe truncuji)
        uint32_t outSse[11] = {};
        GLibpp::Render::convertDepthToGraySSE2(depthVals, outSse, 11);
        check(std::equal(outScalar, outScalar + 11, outSse), "SSE2 varianta == skalar (vc. ocasku)");

        int cpu[4];
        __cpuid(cpu, 7);
        if (cpu[1] & (1 << 5)) // AVX2 - na stroji bez nej vetev preskocime
        {
            uint32_t outAvx[11] = {};
            GLibpp::Render::convertDepthToGrayAVX2(depthVals, outAvx, 11);
            check(std::equal(outScalar, outScalar + 11, outAvx), "AVX2 varianta == skalar (vc. ocasku)");
        }

        runFilteringTests();
    }

    // ---- filtrovani textur: RIP-map urovne, bilinearni tap, vyber urovne ----
    void runFilteringTests()
    {
        using GLibpp::Render::avgARGB;
        using GLibpp::Render::bilinearSampleARGB;
        using GLibpp::Render::buildRipMap;
        using GLibpp::Render::lerpARGB;
        using GLibpp::Render::ripLevelCount;
        using GLibpp::Render::ripLevelSize;
        using GLibpp::Render::RipLevels;

        section("Filtrovani - miseni ARGB (avgARGB, lerpARGB)");

        check(avgARGB(0xFF00FF00u, 0xFF000000u) == 0xFF007F00u,
            "avgARGB pulí kanal bez preteceni do souseda");
        check(avgARGB(0xFFFFFFFFu, 0xFFFFFFFFu) == 0xFFFFFFFFu, "avgARGB dvou shodnych = tentyz");
        check(avgARGB(0xFF010101u, 0xFF000000u) == 0xFF000000u, "avgARGB zaokrouhluje dolu");

        const uint32_t red = Color(255, 0, 0, 255).toRGBA();
        const uint32_t blue = Color(0, 0, 255, 255).toRGBA();
        check(lerpARGB(red, blue, 0) == red, "lerpARGB s vahou 0 vraci prvni texel");
        check(lerpARGB(red, blue, 256) == blue, "lerpARGB s vahou 256 vraci druhy texel");
        check(lerpARGB(red, blue, 128) == Color(127, 0, 127, 255).toRGBA(),
            "lerpARGB s vahou 128 je prumer po kanalech");

        section("Filtrovani - RIP-map urovne (buildRipMap)");

        check(ripLevelCount(512) == 10, "512 ma 10 urovni (512 .. 1)");
        check(ripLevelCount(1) == 1, "jednotkovy rozmer ma jednu uroven");
        check(ripLevelSize(512, 3) == 64, "uroven 3 z 512 je 64");
        check(ripLevelSize(6, 3) == 1, "NPOT rozmer nespadne pod 1 texel");

        // 4x2 textura, hodnoty v modrem kanalu (radky shora):
        //   0x00 0x00 0xFF 0xFF
        //   0x40 0x40 0x80 0x80
        const uint32_t b00 = 0xFF000000u, bFF = 0xFF0000FFu, b40 = 0xFF000040u, b80 = 0xFF000080u;
        const uint32_t base42[8] = { b00, b00, bFF, bFF,
                                     b40, b40, b80, b80 };

        const RipLevels rip = buildRipMap(base42, 4, 2);

        check(rip.levelsU == 3 && rip.levelsV == 2,
            "4x2 -> 3 urovne v u a 2 v v (ma " + std::to_string(rip.levelsU) + "x" + std::to_string(rip.levelsV) + ")");
        // suma pres vsechny urovne = (4+2+1) * (2+1) = 21
        check(rip.texels.size() == 21, "packed velikost je 21 texelu (ma " + std::to_string(rip.texels.size()) + ")");
        check(rip.offsets.size() == 6, "tabulka offsetu ma 6 polozek");
        check(rip.offsets[0] == 0, "uroven (0,0) zacina na offsetu 0");

        // uroven (0,0) je original
        check(std::equal(base42, base42 + 8, rip.texels.begin()), "uroven (0,0) je zdrojovy obrazek");

        // uroven (1,0): pulená sirka -> 2x2, radek 0 = [avg(0,0), avg(FF,FF)]
        const uint32_t* l10 = rip.texels.data() + rip.offsets[1];
        check(l10[0] == b00 && l10[1] == bFF, "uroven (1,0) puli sirku (radek 0)");
        check(l10[2] == b40 && l10[3] == b80, "uroven (1,0) puli sirku (radek 1)");

        // uroven (2,0): 1x2 -> avg(0x00, 0xFF) = 0x7F, avg(0x40, 0x80) = 0x60
        const uint32_t* l20 = rip.texels.data() + rip.offsets[2];
        check(l20[0] == 0xFF00007Fu && l20[1] == 0xFF000060u, "uroven (2,0) je jeden sloupec");

        // uroven (0,1): pulená vyska -> 4x1, avg(0x00,0x40) = 0x20, avg(0xFF,0x80) = 0xBF
        const uint32_t* l01 = rip.texels.data() + rip.offsets[3];
        check(l01[0] == 0xFF000020u && l01[2] == 0xFF0000BFu, "uroven (0,1) puli vysku");

        // NPOT: 6x3 -> urovne 3 x 2, posledni sloupec/radek vypada (floor pravidlo)
        std::vector<uint32_t> base63(18, 0xFF808080u);
        const RipLevels ripNpot = buildRipMap(base63.data(), 6, 3);
        check(ripNpot.levelsU == 3 && ripNpot.levelsV == 2, "6x3 -> 3 x 2 urovne");
        check(ripNpot.texels.size() == (6 + 3 + 1) * (3 + 1), "NPOT packed velikost sedi");

        section("Filtrovani - bilinearni tap");

        // 2x2 textura R G / B W
        const uint32_t quad[4] = {
            Color(255, 0, 0, 255).toRGBA(), Color(0, 255, 0, 255).toRGBA(),
            Color(0, 0, 255, 255).toRGBA(), Color(255, 255, 255, 255).toRGBA(),
        };

        // stred texelu (0,0) je na u = v = 0.25 -> presne ten texel, zadne miseni
        check(bilinearSampleARGB(quad, 2, 2, 0.25f, 0.25f) == quad[0],
            "ve stredu texelu vraci bilinear presne ten texel");
        check(bilinearSampleARGB(quad, 2, 2, 0.75f, 0.75f) == quad[3],
            "ve stredu posledniho texelu take presne");
        // na hranici mezi (0,0) a (1,0) je to jejich prumer
        check(bilinearSampleARGB(quad, 2, 2, 0.5f, 0.25f) == lerpARGB(quad[0], quad[1], 128),
            "na hranici dvou texelu je vysledek jejich prumerem");

        section("Filtrovani - vyber RIP urovne (anizotropie)");

        // Synteticka tabulka urovni pro 8x8: kazda uroven ma vlastni "markerovou"
        // barvu, takze z vysledneho pixelu je poznat, KTERA byla vybrana
        // (vyber urovne se tim testuje nezavisle na box filtru).
        RipLevels marked;
        marked.levelsU = ripLevelCount(8); // 4: 8,4,2,1
        marked.levelsV = ripLevelCount(8);
        marked.offsets.resize(size_t(marked.levelsU) * marked.levelsV);

        uint32_t total = 0;
        for (uint32_t j = 0; j < marked.levelsV; ++j)
            for (uint32_t i = 0; i < marked.levelsU; ++i)
            {
                marked.offsets[size_t(j) * marked.levelsU + i] = total;
                total += ripLevelSize(8, i) * ripLevelSize(8, j);
            }
        marked.texels.resize(total);

        auto markerColor = [](uint32_t i, uint32_t j) { return 0xFF000000u | (i << 8) | j; };
        for (uint32_t j = 0; j < marked.levelsV; ++j)
            for (uint32_t i = 0; i < marked.levelsU; ++i)
            {
                uint32_t* level = marked.texels.data() + marked.offsets[size_t(j) * marked.levelsU + i];
                std::fill_n(level, size_t(ripLevelSize(8, i)) * ripLevelSize(8, j), markerColor(i, j));
            }

        const ShaderUniforms anisoUni{
            1.0f / float(W), 1.0f / float(H),
            marked.texels.data(), 8, 8,
            marked.offsets.data(), marked.levelsU, marked.levelsV
        };

        try
        {
        DeviceTargetDIB anisoTarget(RenderTargetDescriptor::FramebufferRGBA32bit(W, H));
        const auto anisoFn = RasterizerDIB::fragmentFunction(FragmentShaderId::TexturedAniso);
        const Color anyColor(10, 20, 30, 255);

        // stopa 1 texel/pixel v obou osach (UV 0..1 na 8 pixelu, textura 8x8) -> (0,0)
        clearColor(anisoTarget, 0u);
        anisoFn(anisoTarget, nullptr, quadTriSpan(anyColor, 1.0f, 1.0f), anisoUni);
        check(pixelAt(anisoTarget, 3, 2) == markerColor(0, 0),
            "stopa 1 texel/pixel vybere uroven (0,0)");

        // ctyrnasobna stopa jen v u -> uroven (2,0): to je presne ta anizotropie,
        // kterou izotropni mip neumi (zmensil by i v ose v)
        clearColor(anisoTarget, 0u);
        anisoFn(anisoTarget, nullptr, quadTriSpan(anyColor, 4.0f, 1.0f), anisoUni);
        check(pixelAt(anisoTarget, 3, 2) == markerColor(2, 0),
            "ctyrnasobna stopa v u vybere uroven (2,0), v v zustava 0");

        // a symetricky jen ve v
        clearColor(anisoTarget, 0u);
        anisoFn(anisoTarget, nullptr, quadTriSpan(anyColor, 1.0f, 8.0f), anisoUni);
        check(pixelAt(anisoTarget, 3, 2) == markerColor(0, 3),
            "osminasobna stopa v v vybere uroven (0,3)");

        // bez bindnute textury vraci aniso shader fallback barvu instance
        clearColor(anisoTarget, 0u);
        const ShaderUniforms anisoNoTex{ 1.0f / float(W), 1.0f / float(H), nullptr, 0, 0, nullptr, 0, 0 };
        anisoFn(anisoTarget, nullptr, quadTriSpan(anyColor, 1.0f, 1.0f), anisoNoTex);
        check(pixelAt(anisoTarget, 3, 2) == anyColor.toRGBA(),
            "bez textury vraci aniso shader barvu instance");
        }
        catch (const std::exception& e)
        {
            check(false, std::string("vyjimka pri tvorbe targetu: ") + e.what());
        }
    }

}
