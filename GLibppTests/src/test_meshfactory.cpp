// MeshFactory: degenerovane vstupy a integrita index bufferu.
//
// Pozn.: chyba 7 z ANALYZA-CODEBASE.md se nedala "nejdriv otestovat a pak opravit" -
// pri size == 0 podteklo "size - 1" na 4 miliardy iteraci, takze test by se zasekl
// a snedl pamet, misto aby selhal. Ochrany proto vznikly zaroven s temito testy.

#include <string>

#include "MeshFactory.h"

#include "TestRunner.h"

using GLibpp::Geometry::Mesh;
using GLibpp::Geometry::MeshFactory;

namespace {

    // Rasterizer smi predpokladat, ze indexy miri do vertex bufferu a jdou po trojicich
    void checkMeshIntegrity(const Mesh& msh, const std::string& label)
    {
        const size_t vertexCount = msh.getVertexBuffer().size();
        const auto& indices = msh.getIndexBuffer();

        GLibppTests::check(indices.size() % 3 == 0,
            label + ": index buffer je nasobek 3 (ma " + std::to_string(indices.size()) + ")");

        size_t outOfRange = 0;
        for (uint32_t i : indices)
            if (i >= vertexCount) ++outOfRange;

        GLibppTests::check(outOfRange == 0,
            label + ": vsechny indexy miri do vertex bufferu (mimo: " + std::to_string(outOfRange) + ")");
    }

}

namespace GLibppTests {

    void runMeshFactoryTests()
    {
        section("MeshFactory - degenerovane vstupy");

        // Kdyby chybela ochrana, tyhle dva radky se nedobehnou (OOM smycka)
        const Mesh emptyWave = MeshFactory::CreateGridWave(0, 0.2f, 0.0f, 0.05f);
        check(emptyWave.getVertexBuffer().empty() && emptyWave.getIndexBuffer().empty(),
            "CreateGridWave(0) vraci prazdny mesh a nezasekne se");

        const Mesh emptyGrid = MeshFactory::CreateGrid(0, 0.0f);
        check(emptyGrid.getVertexBuffer().empty() && emptyGrid.getIndexBuffer().empty(),
            "CreateGrid(0) vraci prazdny mesh a nezasekne se");

        const Mesh onePointGrid = MeshFactory::CreateGrid(1, 0.0f);
        check(onePointGrid.getIndexBuffer().empty(),
            "CreateGrid(1) nema co triangulovat");

        const Mesh degenerateCylinder = MeshFactory::CreateCylinder(1.0f, 1.0f, 0);
        check(degenerateCylinder.getVertexBuffer().empty(),
            "CreateCylinder(segments = 0) vraci prazdny mesh");

        section("MeshFactory - rozmery a integrita");

        // 4x4 vrcholy = 3x3 quadu = 18 trojuhelniku = 54 indexu
        const Mesh wave4 = MeshFactory::CreateGridWave(4, 0.2f, 0.0f, 0.05f);
        check(wave4.getVertexBuffer().size() == 16,
            "CreateGridWave(4) ma 16 vrcholu (ma " + std::to_string(wave4.getVertexBuffer().size()) + ")");
        check(wave4.getIndexBuffer().size() == 54,
            "CreateGridWave(4) ma 54 indexu (ma " + std::to_string(wave4.getIndexBuffer().size()) + ")");

        checkMeshIntegrity(wave4, "CreateGridWave(4)");
        checkMeshIntegrity(MeshFactory::CreateCube(1.0f), "CreateCube");
        checkMeshIntegrity(MeshFactory::CreateCylinder(1.0f, 2.0f, 16), "CreateCylinder(16)");
        checkMeshIntegrity(MeshFactory::CreateSphere(1.0f, 16), "CreateSphere(16)");
        checkMeshIntegrity(MeshFactory::CreateIcosphere(1.0f, 2), "CreateIcosphere(2)");

        section("MeshFactory - UpdateGridWave");

        // in-place update nesmi menit velikosti a pri nesouhlasnem size nesmi nic delat
        Mesh wave = MeshFactory::CreateGridWave(8, 0.2f, 0.0f, 0.05f);
        const size_t vertexCountBefore = wave.getVertexBuffer().size();
        const float zBefore = wave.getVertexBuffer()[10].z;

        MeshFactory::UpdateGridWave(wave, 8, 0.2f, 3.0f, 0.05f);
        check(wave.getVertexBuffer().size() == vertexCountBefore,
            "UpdateGridWave nemeni velikost vertex bufferu");
        check(!nearlyEqual(wave.getVertexBuffer()[10].z, zBefore),
            "UpdateGridWave se spravnym size zmeni vysku vlny");

        const float zAfter = wave.getVertexBuffer()[10].z;
        MeshFactory::UpdateGridWave(wave, 9, 0.2f, 6.0f, 0.05f); // nesouhlasny size
        check(nearlyEqual(wave.getVertexBuffer()[10].z, zAfter),
            "UpdateGridWave s nesouhlasnym size mesh nechava byt");
    }

}
