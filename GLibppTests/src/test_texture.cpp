// Textury: GDI+ loader (ImageLoaderWin32), registrace v ResourceManageru,
// ShaderTextured (nearest sampling, wrap, fallback bez textury) a perspektivne
// korektni interpolace UV v rasterizeru.

#define NOMINMAX          // ImageLoaderWin32.h je na NOMINMAX pripraveny (Gdiplus trik)
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "RenderTargetDescriptor.h"
#include "RasterizerDIB.h"
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

}

namespace GLibppTests {

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
            const ShaderUniforms uni{ 1.0f / float(W), 1.0f / float(H), texels, 2, 2 };
            const ShaderUniforms noTex{ 1.0f / float(W), 1.0f / float(H), nullptr, 0, 0 };

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
            const ShaderUniforms rtUni{ 1.0f / float(W), 1.0f / float(H), rt.framebuffer, uint32_t(W), uint32_t(H) };
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
    }

}
