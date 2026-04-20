#pragma once

#include <cassert>
template<typename T>
class SlotArray {
public:

    struct Handle {
        size_t index;
        uint32_t generation;
    };

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
        return h.index < items.size() &&
            items[h.index] != nullptr &&
            generations[h.index] == h.generation;
    }

    T& get(Handle h) {
        assert(isValid(h));
        return *items[h.index];
    }

    const T& get(Handle h) const {
        assert(isValid(h));
        return *items[h.index];
    }

private:
    std::vector<std::unique_ptr<T>> items;
    std::vector<uint32_t> generations;
    std::vector<size_t> freeList;
};