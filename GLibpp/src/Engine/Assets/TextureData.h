#pragma once

#include <cstdint>
#include <vector>

namespace GLibpp::Assets {

    // Kanonicka pixelova data textury: ARGB 0xAARRGGBB, radky shora dolu -
    // stejny layout jako framebuffer DIB backendu, takze residency upload je
    // proste kopie. Plni je platformovy image loader (Platform::ImageLoaderWin32),
    // vlastni je ResourceManager; backend residency (ShaderResource target,
    // budouci GL textura) je vec konkretniho Device, klicovana handlem.
    struct TextureData {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint32_t> pixels; // width * height texelu

        // Obsah se meni za behu (capture textury, render-to-texture): kanonicka
        // data jsou pak jen zakladatel residency a zustavaji zamerne stale.
        // Backend pro takovou texturu NESTAVI filtracni urovne (RIP-map) -
        // po prvnim framu by lhaly a prestavba kazdy frame je drahá.
        bool dynamic = false;

        bool isValid() const noexcept {
            return width > 0 && height > 0
                && pixels.size() == size_t(width) * height;
        }
    };

}
