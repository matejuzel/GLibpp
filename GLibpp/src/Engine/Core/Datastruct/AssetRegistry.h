#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <limits>
#include "StableRegistry.h"

template<typename T>
class AssetRegistry
{
public:
    using Handle = typename StableRegistry<T>::Handle;

    static constexpr Handle invalidHandle{ std::numeric_limits<size_t>::max(), 0 };

    // Přidání assetu bez jména – vygeneruje se automaticky
    Handle add(const T& value)
    {
        std::string name = "asset_" + std::to_string(counter++);
        return add(name, value);
    }

    Handle add(T&& value)
    {
        std::string name = "asset_" + std::to_string(counter++);
        return add(name, std::move(value));
    }

    // Přidání assetu se jménem
    Handle add(const std::string& name, const T& value)
    {
        if (name_to_handle.contains(name))
            return invalidHandle;

        Handle h = slots.add(value);
        name_to_handle[name] = h;
        return h;
    }

    Handle add(const std::string& name, T&& value)
    {
        if (name_to_handle.contains(name))
            return invalidHandle;

        Handle h = slots.add(std::move(value));
        name_to_handle[name] = h;
        return h;
    }

    // Získání assetu podle handle
    T* get(Handle h)
    {
        return slots.isValid(h) ? &slots.get(h) : nullptr;
    }

    const T* get(Handle h) const
    {
        return slots.isValid(h) ? &slots.get(h) : nullptr;
    }

    // Získání assetu podle jména
    T* get(const std::string& name)
    {
        auto it = name_to_handle.find(name);
        return it != name_to_handle.end() ? get(it->second) : nullptr;
    }

    const T* get(const std::string& name) const
    {
        auto it = name_to_handle.find(name);
        return it != name_to_handle.end() ? get(it->second) : nullptr;
    }

    // Odstranění assetu podle handle
    void remove(Handle h)
    {
        if (!slots.isValid(h))
            return;

        for (auto it = name_to_handle.begin(); it != name_to_handle.end(); ++it)
        {
            if (it->second.index == h.index &&
                it->second.generation == h.generation)
            {
                name_to_handle.erase(it);
                break;
            }
        }

        slots.remove(h);
    }

    // Iterace přes všechny validní assety
    template<typename F>
    void iterate(F&& callback)
    {
        slots.iterate(std::forward<F>(callback));
    }

private:
    StableRegistry<T> slots;
    std::unordered_map<std::string, Handle> name_to_handle;
    size_t counter = 0;
};
