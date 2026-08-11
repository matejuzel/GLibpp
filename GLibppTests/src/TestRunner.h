#pragma once

#include <cstdio>
#include <string>

// Minimalisticky test runner - zadny framework, jen pocitadlo a vypis.
// Konvence: kazda sada je jedna funkce runXxxTests() volana z main.cpp,
// uvnitr se pouziva section() pro nadpis a check() pro jedno tvrzeni.
// Exit kod = 0 kdyz vsechno proslo (aby to slo zaradit do commit gate).

namespace GLibppTests {

    inline int g_checks = 0;
    inline int g_failed = 0;

    inline void section(const char* name)
    {
        std::printf("\n--- %s ---\n", name);
    }

    inline void check(bool ok, const std::string& what)
    {
        ++g_checks;
        if (!ok) ++g_failed;
        std::printf("%s  %s\n", ok ? "[ OK ]" : "[FAIL]", what.c_str());
    }

    // tolerance na float srovnani - matematika se pocita v float, ne v double
    inline bool nearlyEqual(float a, float b, float eps = 1e-4f)
    {
        const float d = a - b;
        return (d < 0.0f ? -d : d) <= eps;
    }

    // jednotlive sady (definice v prislusnych .cpp)
    void runRasterizerTests();
    void runMathTests();
    void runMeshFactoryTests();

}
