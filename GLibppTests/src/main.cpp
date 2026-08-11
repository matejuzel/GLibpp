// Testy GLibpp - konzolovy runner, x64 only (viz CLAUDE.md).
// Testuje SKUTECNE hlavicky enginu, ne kopie algoritmu.

#include "TestRunner.h"

int main()
{
    using namespace GLibppTests;

    std::printf("GLibpp testy\n");

    runRasterizerTests();
    runShaderTests();
    runTextureTests();
    runMathTests();
    runMeshFactoryTests();

    std::printf("\n%d kontrol, %d selhalo\n", g_checks, g_failed);
    std::printf("%s\n", g_failed == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");

    return g_failed == 0 ? 0 : 1;
}
