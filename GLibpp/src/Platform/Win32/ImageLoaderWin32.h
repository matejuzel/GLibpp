#pragma once

#include "TextureData.h"

#include <windows.h>

// gdiplus.h pouziva min/max bez kvalifikace - s NOMINMAX (testy) by se
// nerozbalila makra, proto se mu dodaji std varianty
#include <algorithm>
namespace Gdiplus { using std::min; using std::max; }
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include <stdexcept>
#include <string>

namespace GLibpp::Platform {

    // Nacteni obrazku ze souboru do kanonickych ARGB dat pres GDI+.
    // GDI+ je soucast Windows (zadna externi zavislost - stejna filozofie jako
    // DIB/DWM) a dekoduje JPEG, PNG, BMP i GIF. Bezi jen v setup fazi, takze
    // startup/shutdown GDI+ per volani nevadi.
    struct ImageLoaderWin32 {

        static Assets::TextureData Load(const std::string& path)
        {
            struct GdiplusSession {
                ULONG_PTR token = 0;
                GdiplusSession() {
                    Gdiplus::GdiplusStartupInput input;
                    if (Gdiplus::GdiplusStartup(&token, &input, nullptr) != Gdiplus::Ok)
                        throw std::runtime_error("GdiplusStartup selhal");
                }
                ~GdiplusSession() { Gdiplus::GdiplusShutdown(token); }
            } session;

            Gdiplus::Bitmap bitmap(toWide(path).c_str());
            if (bitmap.GetLastStatus() != Gdiplus::Ok)
                throw std::runtime_error("Nepodarilo se nacist texturu: " + path);

            Assets::TextureData tex;
            tex.width = bitmap.GetWidth();
            tex.height = bitmap.GetHeight();
            if (tex.width == 0 || tex.height == 0)
                throw std::runtime_error("Textura ma nulovy rozmer: " + path);

            tex.pixels.resize(size_t(tex.width) * tex.height);

            // LockBits s UserInputBuf prekonvertuje pixely primo do naseho bufferu;
            // PixelFormat32bppARGB = 0xAARRGGBB v little-endian DWORDu - presne
            // layout Color::toRGBA() i DIB framebufferu
            Gdiplus::Rect rect(0, 0, int(tex.width), int(tex.height));
            Gdiplus::BitmapData data{};
            data.Width = tex.width;
            data.Height = tex.height;
            data.Stride = int(tex.width * 4);
            data.PixelFormat = PixelFormat32bppARGB;
            data.Scan0 = tex.pixels.data();

            if (bitmap.LockBits(&rect,
                    Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf,
                    PixelFormat32bppARGB, &data) != Gdiplus::Ok)
                throw std::runtime_error("Nepodarilo se precist pixely textury: " + path);

            bitmap.UnlockBits(&data);

            return tex;
        }

    private:

        static std::wstring toWide(const std::string& s)
        {
            if (s.empty()) return {};
            const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.size()), nullptr, 0);
            std::wstring w(size_t(len), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.size()), w.data(), len);
            return w;
        }
    };

}
