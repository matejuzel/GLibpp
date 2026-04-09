#pragma once

#include <vector>
#include <cstdint>
#include <cassert>
#include "MeshInstance.h"

/*
class MeshInstanceRegistry {
public:
    static constexpr MeshInstanceHandle INVALID = { UINT32_MAX, UINT32_MAX };

    MeshInstanceRegistry() = default;

    MeshInstanceHandle create(mesh_handle_t mesh, material_handle_t material, const Mtx4& transform)
    {
        uint32_t index;

        if (!freeList.empty()) {
            index = freeList.back();
            freeList.pop_back();

            Slot& slot = slots[index];
            slot.generation++;
            slot.alive = true;

            // explicitní konstrukce pøímo ve slotu
            new (&slot.instance) MeshInstance(mesh, material, transform);

        }
        else {
            //Slot slot;
            //slot.generation = 0;
            //slot.alive = true;
            //new (&slot.instance) MeshInstance(mesh, material, transform);
            //slots.push_back(std::move(slot));


            slots.emplace_back();
            Slot& slotRef = slots.back();
            slotRef.generation = 0;
            slotRef.alive = true;

            new (&slotRef.instance) MeshInstance(mesh, material, transform);



            index = slots.size() - 1;
        }

        return MeshInstanceHandle{ index, slots[index].generation };
    }


    // Získání instance (bezpeèné pøes generation index)
    MeshInstance& get(const MeshInstanceHandle& h)
    {
        assert(isValid(h));
        return slots[h.index].instance;
    }

    const MeshInstance& get(const MeshInstanceHandle& h) const
    {
        assert(isValid(h));
        return slots[h.index].instance;
    }

    // Bezpeèné odstranìní instance
    void destroy(const MeshInstanceHandle& h)
    {
        if (!isValid(h))
            return;

        Slot& slot = slots[h.index];
        slot.instance.~MeshInstance(); // explicitní destrukce
        slot.alive = false;

        freeList.push_back(h.index);
    }

    bool isValid(const MeshInstanceHandle& h) const
    {
        return h.index < slots.size() &&
            slots[h.index].alive &&
            slots[h.index].generation == h.generation;
    }

    size_t size() const { return slots.size(); }

private:
    struct Slot {
        MeshInstance instance; // uložený pøímo, žádné pointery
        uint32_t generation = 0;
        bool alive = false;
    };

    std::vector<Slot> slots;
    std::vector<uint32_t> freeList;
};
*/