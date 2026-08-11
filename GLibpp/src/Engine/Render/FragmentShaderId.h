#pragma once

#include <cstdint>

namespace GLibpp::Render {

    // Identita fragment shaderu - backend-agnosticka, stejna filozofie jako
    // mesh handly: engine zna jen identitu, backend rozhoduje, co znamena
    // (DIB: ukazatel na instanciaci rasterizacni smycky, budouci GL: program).
    //
    // Vedome enum, ne StableRegistry::Handle: shadery jsou KOD - mnozina
    // uzavrena v compile time bez zivotniho cyklu (neni co registrovat ani
    // rusit, generace by lhala). Enum + static_assert na Count dava uz pri
    // prekladu garanci, ze kazde id ma implementaci (viz kFragmentDispatch
    // v RasterizerDIB). Handle system se shadery potka az o vrstvu vys:
    // budouci MaterialHandle = DATA (id shaderu + parametry) v registru.
    enum class FragmentShaderId : uint8_t {
        Solid,    // passthrough - zakladni barva instance
        Lambert,  // ploche Lambertovo stinovani (vychozi vzhled dema)
        UvDebug,  // vizualizace normalizovanych souradnic obrazovky (r = u, g = v)
        Textured, // nearest vzorkovani bindnute textury na perspektivnich UV
        Count     // pocet - neni to shader
    };

}
