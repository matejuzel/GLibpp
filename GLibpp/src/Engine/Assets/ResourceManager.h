#pragma once

#include "Mesh.h"
#include "MeshInstance.h"
#include "ResourceHandles.h"
#include "StableRegistry.h"
#include "Mtx4.h"
#include "Color.h"

#include <cassert>
#include <utility>

namespace GLibpp::Assets {

    // Kanonicke uloziste assetu (meshe, instance) + jejich identita (handly).
    // Netemplatovana trida bez znalosti backendu - residency (kopie geometrie,
    // offsety, VBO, ...) je privatni vec konkretniho Device, klicovana handlem.
    //
    // Vlastni ho App (assety = obsah aplikace); Renderer dostava referenci a orchestruje:
    // na zacatku runLoop zavola freeze() a upload walk (meshForEach -> device.meshRegister).
    //
    // Vlaknovy kontrakt: registrace POUZE pred startem render vlakna (registry neni
    // thread-safe); po freeze() na data saha vyhradne render vlakno (vc. meshGetDynamic).
    // Budouci runtime tvorba z logickeho vlakna pujde pres SPSC upload frontu
    // konzumovanou na render vlakne - verejne API se tim nezmeni.
    struct ResourceManager {

        // vraci handle okamzite; volajici nesmi predpokladat synchronni rezidenci dat v backendu
        MeshHandle meshRegister(Geometry::Mesh mesh)
        {
            assert(!frozen && "registrace resources jen pred startem render smycky (runLoop)");
            return meshes.add(std::move(mesh));
        }

        const Geometry::Mesh& meshGet(MeshHandle h) const
        {
            return meshes.get(h);
        }

        // mutable pristup pro dynamicke meshe (napr. GridWave) - jen render vlakno,
        // in-place mutace dat bez alokaci; po zmene je nutne notifikovat backend (device.meshUpdate)
        Geometry::Mesh& meshGetDynamic(MeshHandle h)
        {
            return meshes.get(h);
        }

        bool meshIsValid(MeshHandle h) const
        {
            return meshes.isValid(h);
        }

        // jen pred freeze - backend mesh jeste nevidel (upload probiha az pri freeze v runLoop)
        void meshRemove(MeshHandle h)
        {
            assert(!frozen && "meshRemove jen pred startem render smycky (runLoop)");
            meshes.remove(h);
        }

        // iterace pres vsechny meshe (handle + data) - upload walk na zacatku runLoop
        template<typename F>
        void meshForEach(F&& f) const
        {
            meshes.forEach(std::forward<F>(f));
        }

        MeshInstanceHandle meshInstanceRegister(MeshHandle mesh,
                                                const Mtx4& localTransform = Mtx4::Identity(),
                                                Render::Color color = Render::Color::Grayscale(0.3f),
                                                bool wireframe = false)
        {
            assert(!frozen && "registrace resources jen pred startem render smycky (runLoop)");
            assert(meshes.isValid(mesh));
            return instances.add(Geometry::MeshInstance{ mesh, localTransform, color, wireframe });
        }

        const Geometry::MeshInstance& meshInstanceGet(MeshInstanceHandle h) const
        {
            return instances.get(h);
        }

        bool meshInstanceIsValid(MeshInstanceHandle h) const
        {
            return instances.isValid(h);
        }

        // vola render vlakno na zacatku runLoop - od te chvile je registrace zakazana
        void freeze()
        {
            frozen = true;
        }

    private:

        Core::StableRegistry<Geometry::Mesh> meshes;
        Core::StableRegistry<Geometry::MeshInstance> instances;
        bool frozen = false;
    };

}
