#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace GLibpp::Render {

    // ---- RIP-map (rectangular mip map): filtracni urovne textury ----
    //
    // Klasicky mip chain zmensuje OBE osy zaraz (W/2^i x H/2^i), takze uroven
    // se musi vybrat podle DELSI osy stopy pixelu - na sikme plose (podlaha,
    // vozovka) tim zahodi detail i ve smeru, kde je stopa uzka. To je to
    // rozmazani, ktere na GPU resi anizotropni filtrace N vzorky podel delsi
    // osy: kvalita za cenu az 16x vic tapu na pixel.
    //
    // RIP-map obraci kompromis: predpocita 2D TABULKU urovni W/2^i x H/2^j
    // s NEZAVISLYM delenim v u a v, takze anizotropie je zapecena v datech
    // a sampler zvladne jeden lookup. Cena je pamet - suma pres vsechny
    // urovne je (2W)(2H) = 4*W*H misto 1,33*W*H u mipu.
    //
    // Proc prave tady: na CPU rasterizeru je vzacna per-pixel prace, ne pamet
    // (zmereny zakon o Debug rozpoctu - vyplneni ~poloviny okna nestiha uz
    // pri jednom tapu), takze u nas vychodni GPU kompromis nesedi.
    // Omezeni: resi jen anizotropii ZAROVNANOU s osami u/v; diagonalni
    // elongace vybere konzervativnejsi uroven (mirne rozmaze). Diagonala
    // je domena EWA / N-tap vzorkovani - vedome mimo scope.
    //
    // Diagonala tabulky (i == j) JE klasicky mip chain, takze izotropni
    // pripad z toho padne zdarma.
    //
    // Layout: vsechny urovne v jednom souvislem poli + tabulka offsetu
    // indexovana [j * levelsU + i] - stejny offsetovy idiom jako residency
    // meshu (meshRanges). Rozmery urovni se neukladaji, dopocitavaji se
    // z bazovych pres ripLevelSize.

    // rozmer urovne: pulení, ale nikdy pod 1 texel (NPOT textury)
    inline uint32_t ripLevelSize(uint32_t base, uint32_t level) noexcept
    {
        const uint32_t s = base >> level;
        return s > 0 ? s : 1u;
    }

    // pocet urovni v jedne ose: az po uroven o jednom texelu vcetne
    // (512 -> 10 urovni: 512, 256, ..., 1)
    inline uint32_t ripLevelCount(uint32_t base) noexcept
    {
        uint32_t n = 1;
        while ((base >> n) > 0) ++n;
        return n;
    }

    // Prumer dvou ARGB texelu po kanalech, bez preteceni mezi kanaly.
    // Pouziva SWAR identitu a + b == (a ^ b) + 2 * (a & b), takze
    // (a + b) / 2 == ((a ^ b) >> 1) + (a & b); maska 0xFEFEFEFE zahodi
    // nejnizsi bit kazdeho kanalu pred posunem, aby neprosakl do kanalu
    // pod nim. Vysledek je presne floor((a + b) / 2) po kanalech.
    inline uint32_t avgARGB(uint32_t a, uint32_t b) noexcept
    {
        return (((a ^ b) & 0xFEFEFEFEu) >> 1) + (a & b);
    }

    // pulení sirky box filtrem: dva vodorovni sousedi -> jeden texel.
    // Pri neparne sirce posledni sloupec vypadne (standardni floor pravidlo).
    inline void ripHalveHorizontal(const uint32_t* src, uint32_t srcWidth, uint32_t height,
                                   uint32_t* dst, uint32_t dstWidth) noexcept
    {
        for (uint32_t y = 0; y < height; ++y)
        {
            const uint32_t* s = src + size_t(y) * srcWidth;
            uint32_t* d = dst + size_t(y) * dstWidth;

            if (srcWidth == 1) { d[0] = s[0]; continue; } // uz nelze pulit

            for (uint32_t x = 0; x < dstWidth; ++x)
                d[x] = avgARGB(s[2 * x], s[2 * x + 1]);
        }
    }

    // pulení vysky box filtrem (dva svisli sousedi -> jeden texel)
    inline void ripHalveVertical(const uint32_t* src, uint32_t width, uint32_t srcHeight,
                                 uint32_t* dst, uint32_t dstHeight) noexcept
    {
        for (uint32_t y = 0; y < dstHeight; ++y)
        {
            const uint32_t row0 = (srcHeight == 1) ? 0u : 2 * y;
            const uint32_t row1 = (srcHeight == 1) ? 0u : 2 * y + 1;

            const uint32_t* s0 = src + size_t(row0) * width;
            const uint32_t* s1 = src + size_t(row1) * width;
            uint32_t* d = dst + size_t(y) * width;

            for (uint32_t x = 0; x < width; ++x)
                d[x] = avgARGB(s0[x], s1[x]);
        }
    }

    // Filtracni urovne jedne textury - privatni residency detail backendu
    // (kanonicka TextureData nese jen zdrojovy obrazek).
    struct RipLevels {
        uint32_t levelsU = 0;
        uint32_t levelsV = 0;
        std::vector<uint32_t> offsets; // [j * levelsU + i] -> offset do texels
        std::vector<uint32_t> texels;  // vsechny urovne za sebou

        bool empty() const noexcept { return texels.empty(); }
    };

    // Postavi celou tabulku urovni ze zdrojoveho obrazku (ARGB, radky shora).
    // Vola se jednou pri uploadu textury, ne za behu.
    inline RipLevels buildRipMap(const uint32_t* base, uint32_t width, uint32_t height)
    {
        RipLevels rip;
        if (base == nullptr || width == 0 || height == 0) return rip;

        rip.levelsU = ripLevelCount(width);
        rip.levelsV = ripLevelCount(height);
        rip.offsets.resize(size_t(rip.levelsU) * rip.levelsV);

        // 1) rozvrzeni: offsety a celkova velikost
        uint32_t total = 0;
        for (uint32_t j = 0; j < rip.levelsV; ++j)
        {
            for (uint32_t i = 0; i < rip.levelsU; ++i)
            {
                rip.offsets[size_t(j) * rip.levelsU + i] = total;
                total += ripLevelSize(width, i) * ripLevelSize(height, j);
            }
        }
        rip.texels.resize(total);

        // 2) uroven (0,0) je original
        std::copy(base, base + size_t(width) * height, rip.texels.begin());

        // 3) sloupec i == 0: pulení vysky z (0, j-1)
        for (uint32_t j = 1; j < rip.levelsV; ++j)
        {
            ripHalveVertical(
                rip.texels.data() + rip.offsets[size_t(j - 1) * rip.levelsU], width,
                ripLevelSize(height, j - 1),
                rip.texels.data() + rip.offsets[size_t(j) * rip.levelsU],
                ripLevelSize(height, j));
        }

        // 4) zbytek: pulení sirky z (i-1, j) - kazda uroven z uz hotoveho souseda,
        //    takze cela tabulka stoji O(4 * W * H) prace
        for (uint32_t j = 0; j < rip.levelsV; ++j)
        {
            const uint32_t h = ripLevelSize(height, j);
            for (uint32_t i = 1; i < rip.levelsU; ++i)
            {
                ripHalveHorizontal(
                    rip.texels.data() + rip.offsets[size_t(j) * rip.levelsU + (i - 1)],
                    ripLevelSize(width, i - 1), h,
                    rip.texels.data() + rip.offsets[size_t(j) * rip.levelsU + i],
                    ripLevelSize(width, i));
            }
        }

        return rip;
    }

}
