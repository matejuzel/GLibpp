#pragma once

#include <vector>
#include <memory>
#include <limits>
#include <cassert>
#include <cstdint>
template<typename T>
class HandleRegistry {
public:

    struct Handle {
        size_t index;
        uint32_t generation;

        bool operator==(const Handle& other) const 
        {
            return index == other.index && generation == other.generation;
        }

    };

    static constexpr Handle INVALID{std::numeric_limits<uint32_t>::max(), 0};

    template<typename... Args>
    Handle add(Args&&... args) {
        size_t index;

        if (!freeList.empty()) {
            index = freeList.back();
            freeList.pop_back();
            items[index] = std::make_unique<T>(std::forward<Args>(args)...);
        }
        else {
            index = items.size();
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
        return h.index < items.size() && items[h.index] != nullptr && generations[h.index] == h.generation;
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
    std::vector<size_t> freeList;
};