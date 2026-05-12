#pragma once

#include <utility>

template <typename T>
class ZeroAllocStateHistory {
public:
    ZeroAllocStateHistory() {
        history[0] = T();
        history[1] = T();
    }

    explicit ZeroAllocStateHistory(const T& initial_value) {
        history[0] = initial_value;
        history[1] = initial_value;
    }

    // Zakážeme kopírování/pøesouvání držáku historie
    ZeroAllocStateHistory(const ZeroAllocStateHistory&) = delete;
    ZeroAllocStateHistory& operator=(const ZeroAllocStateHistory&) = delete;
    ZeroAllocStateHistory(ZeroAllocStateHistory&&) = delete;
    ZeroAllocStateHistory& operator=(ZeroAllocStateHistory&&) = delete;

    // --- MANIPULACE A ZÁPIS ---

    // Posune historii a rovnou vrátí referenci na nový aktuální stav pro zápis
    T& advance_and_get_current() {
        std::swap(previous_idx, current_idx);
        return history[current_idx];
    }

    void advance_and_load_current(const T& state)
    {
        std::swap(previous_idx, current_idx);
        history[current_idx] = state;
    }

    void advance_and_load_current(T&& state)
    {
        std::swap(previous_idx, current_idx);
        history[current_idx] = std::move(state);
    }


    // --- PØÍSTUP PRO ÈTENÍ ---

    const T& get_previous() const {
        return history[previous_idx];
    }

    const T& get_current() const {
        return history[current_idx];
    }

private:
    T history[2];
    int previous_idx = 0;
    int current_idx = 1;
};