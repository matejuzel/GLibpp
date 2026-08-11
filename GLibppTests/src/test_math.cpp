// Matematika: konvence projekci, cistota Vec4::cross, interpolace pres ADL.
//
// Vsechna volani Lerp/Slerp jsou zamerne NEKVALIFIKOVANA - je to zaroven test
// konvence "skryty friend nalezeny pres ADL" (viz CLAUDE.md).

#include <cmath>
#include <algorithm>
#include <string>

#include "Vec4.h"
#include "Mtx4.h"
#include "Quaternion.h"
#include "Mathematics.h"

#include "TestRunner.h"

namespace {

    // NDC hloubka bodu, ktery lezi v pohledovem prostoru ve vzdalenosti dist
    // pred kamerou (kamera hledi po -Z, takze view z = -dist)
    float ndcDepthAt(const Mtx4& projection, float dist)
    {
        Vec4 clip = projection * Vec4(0.0f, 0.0f, -dist, 1.0f);
        clip.divideW();
        return clip.z;
    }

    bool hasNonFinite(const Mtx4& m)
    {
        const float* d = m.getRawData();
        for (int i = 0; i < 16; ++i)
            if (!std::isfinite(d[i])) return true;
        return false;
    }

}

namespace GLibppTests {

    void runMathTests()
    {
        section("Projekce - konvence clip-space Z");

        const float nearZ = 0.5f;
        const float farZ = 100.0f;

        // Na tomhle stoji depth buffer: kDepthFar = 1.0f a test "mensi vyhrava"
        const Mtx4 persp = Mtx4::Perspective(GLibpp::Math::deg2rad(60.0f), 16.0f / 9.0f, nearZ, farZ);
        check(nearlyEqual(ndcDepthAt(persp, nearZ), -1.0f), "Perspective: near rovina -> NDC z = -1");
        check(nearlyEqual(ndcDepthAt(persp, farZ), 1.0f), "Perspective: far rovina -> NDC z = +1");
        check(ndcDepthAt(persp, 1.0f) < ndcDepthAt(persp, 10.0f),
            "Perspective: hloubka roste se vzdalenosti (blizsi = mensi z)");

        // Orthographic musi mit STEJNOU konvenci jako Perspective, jinak se pri
        // prepnuti projekce obrati depth test (chyba 8 z ANALYZA-CODEBASE.md)
        const Mtx4 ortho = Mtx4::Orthographic(-10.0f, 10.0f, -10.0f, 10.0f, nearZ, farZ);
        check(nearlyEqual(ndcDepthAt(ortho, nearZ), -1.0f), "Orthographic: near rovina -> NDC z = -1");
        check(nearlyEqual(ndcDepthAt(ortho, farZ), 1.0f), "Orthographic: far rovina -> NDC z = +1");
        check(ndcDepthAt(ortho, 1.0f) < ndcDepthAt(ortho, 10.0f),
            "Orthographic: hloubka roste se vzdalenosti (stejny smysl jako Perspective)");

        section("Mtx4 - inverze singularni matice");

        // Skalovani nulou ma singularni 3x3 blok - nesmi z toho vypadnout inf/NaN
        const Mtx4 singular = Mtx4::Scaling(0.0f);
        check(!hasNonFinite(singular.invertedAffine()),
            "inverseAffine na singularni matici nevyrobi inf/NaN");
        check(!hasNonFinite(singular.inverted()),
            "inverse na singularni matici nevyrobi inf/NaN");

        section("Vec4 - cistota cross a interpolace pres ADL");

        // regrese: cross byval mutujici (Vec4&) a tise prepisoval prijemce
        Vec4 ex(1.0f, 0.0f, 0.0f, 0.0f);
        const Vec4 ey(0.0f, 1.0f, 0.0f, 0.0f);
        const Vec4 crossResult = ex.cross(ey);

        check(nearlyEqual(ex.x, 1.0f) && nearlyEqual(ex.y, 0.0f) && nearlyEqual(ex.z, 0.0f),
            "Vec4::cross nemodifikuje prijemce");
        check(nearlyEqual(crossResult.z, 1.0f), "cross(x, y) miri po +z");

        const Vec4 mid = Lerp(Vec4(0.0f, 0.0f, 0.0f, 1.0f), Vec4(10.0f, 20.0f, 30.0f, 1.0f), 0.5f);
        check(nearlyEqual(mid.x, 5.0f) && nearlyEqual(mid.y, 10.0f) && nearlyEqual(mid.z, 15.0f),
            "Lerp(Vec4) nalezena pres ADL a interpoluje po slozkach");

        section("Quaternion - Slerp");

        const Quaternion qa = Quaternion::RotationY(0.0f);
        const Quaternion qb = Quaternion::RotationY(GLibpp::Math::deg2rad(90.0f));

        const Vec4 probe(0.0f, 0.0f, 1.0f, 0.0f);
        const Vec4 v0 = qa * probe;
        const Vec4 v1 = qb * probe;
        const Vec4 vm = Slerp(qa, qb, 0.5f) * probe;

        auto angleBetween = [](const Vec4& a, const Vec4& b) {
            return std::acos(std::clamp(Vec4::dot(a, b), -1.0f, 1.0f));
            };

        // definicni vlastnost slerpu: konstantni uhlova rychlost
        check(nearlyEqual(angleBetween(v0, vm), angleBetween(v0, v1) * 0.5f, 1e-3f),
            "Slerp(Quaternion) je v t=0.5 presne v pulce uhlu");
        check(nearlyEqual(Slerp(qa, qb, 0.5f).length(), 1.0f),
            "Slerp(Quaternion) vraci jednotkovy quaternion");

        section("Mtx4 - Slerp baze");

        // regrese: Mtx4::Slerp vracel nesmysl, dokud byl Vec4::cross mutujici
        const Mtx4 mSlerp = Slerp(Mtx4::Identity(), Mtx4::RotationY(GLibpp::Math::deg2rad(90.0f)), 0.5f);

        auto columnLength = [&](int col) {
            const Vec4 c(mSlerp.at(0, col), mSlerp.at(1, col), mSlerp.at(2, col), 0.0f);
            return c.length();
            };

        check(nearlyEqual(columnLength(0), 1.0f, 1e-3f)
            && nearlyEqual(columnLength(1), 1.0f, 1e-3f)
            && nearlyEqual(columnLength(2), 1.0f, 1e-3f),
            "Slerp(Mtx4) vraci ortonormalni bazi (jednotkove sloupce)");
    }

}
