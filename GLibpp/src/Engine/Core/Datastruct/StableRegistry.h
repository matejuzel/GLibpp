#pragma once

#include <vector>
#include <memory>
#include <limits>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

namespace GLibpp::Core {

template<typename T>
class StableRegistry {
public:

    struct Handle {
        // default = INVALID, aby default-konstruovany handle nikdy omylem nemiril na slot 0
        uint32_t index = std::numeric_limits<uint32_t>::max();
        uint32_t generation = 0;

        bool operator==(const Handle& other) const
        {
            return index == other.index && generation == other.generation;
        }

        friend std::ostream& operator<<(std::ostream& os, const Handle& h) {
            return os << "Handle(index=" << h.index << ", generation=" << h.generation << ")";
        }
    };

    static constexpr Handle INVALID{ std::numeric_limits<uint32_t>::max(), 0 };


    StableRegistry() = default;

    StableRegistry(const StableRegistry&) = delete;
    StableRegistry& operator=(const StableRegistry&) = delete;

    template<typename... Args>
    Handle add(Args&&... args) {
        uint32_t index;

        if (!freeList.empty()) {
            index = freeList.back();
            freeList.pop_back();
            items[index] = std::make_unique<T>(std::forward<Args>(args)...);
        }
        else {
            assert(items.size() < std::numeric_limits<uint32_t>::max());
            index = static_cast<uint32_t>(items.size());
            items.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
            generations.push_back(0);
        }

        return Handle{ index, generations[index] };
    }

    void remove(Handle h) {
        if (!isValid(h)) return;

        items[h.index].reset();
        generations[h.index]++;
        freeList.push_back(h.index);
    }

    bool isValid(Handle h) const {
        if (h == INVALID) return false;
        if (h.index >= items.size()) return false;
        return items[h.index] != nullptr && generations[h.index] == h.generation;
    }

    // iterace pres vsechny zive polozky (handle + data) - napr. upload geometrie do backendu
    template<typename F>
    void forEach(F&& f) const {
        for (uint32_t i = 0; i < items.size(); ++i) {
            if (items[i]) f(Handle{ i, generations[i] }, *items[i]);
        }
    }

    T& get(Handle h) {
        assert(isValid(h));
        return *items[h.index];
    }

    const T& get(Handle h) const {
        assert(isValid(h));
        return *items[h.index];
    }

    template<typename... Args>
    void reset(Handle h, Args&&... args) {
        assert(isValid(h));

        // znič starý objekt
        items[h.index].reset();

        // vytvoř nový objekt na stejném místě
        items[h.index] = std::make_unique<T>(std::forward<Args>(args)...);

        // generace se NEMĚNÍ → handle zůstává validní
    }
    

private:
    std::vector<std::unique_ptr<T>> items;
    std::vector<uint32_t> generations;
    std::vector<uint32_t> freeList;
};

}

