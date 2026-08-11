#pragma once

#include "FragmentShader.h"
#include <cstdint>

namespace GLibpp::Render {

    // Debug vizualizace normalizovanych souradnic obrazovky: r = u, g = v
    // (klasicky "UV debug" gradient - vlevo nahore cerna, vpravo cervena,
    // dole zelena). Normalizaci si shader odvozuje z uniformu, pixel zustava
    // primitiv - viz komentar u ShaderUniforms.
    //
    // Zaroven je to prvni shader, ktery pocita SKUTECNE per pixel (TriCtx je
    // prazdny, vsechna prace v shade) - overuje, ze per-pixel cesta inlinovana
    // do rasterizacni smycky funguje a ze uniformy dotecou az tam.
    struct ShaderUvDebug {

        struct TriCtx {
        };

        static TriCtx setup(const TriangleInput&, const ShaderUniforms&) noexcept
        {
            return {};
        }

        static uint32_t shade(const TriCtx&, const PixelInput& px, const ShaderUniforms& uni) noexcept
        {
            // stred pixelu -> [0, 1] (ShaderToy konvence: uv = fragCoord / iResolution)
            const float u = (float(px.x) + 0.5f) * uni.invWidth;
            const float v = (float(px.y) + 0.5f) * uni.invHeight;

            return Color(
                uint8_t(u * 255.0f),
                uint8_t(v * 255.0f),
                0,
                255
            ).toRGBA();
        }
    };

}
