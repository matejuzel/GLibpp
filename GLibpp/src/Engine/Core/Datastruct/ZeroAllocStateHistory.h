#pragma once

#include <cstddef>
#include <utility>

namespace GLibpp::Core {

// Ring poslednich N stavu (bez alokaci) - historie pro snapshot interpolaci.
// N = 2 je klasicky prev/curr par; vetsi N umoznuje vybrat par stavu, ktery
// cilovy (vizualni) cas skutecne obklopuje, i kdyz se render drzi vic nez
// jeden tick za simulaci (viz kInterpDelayTicks v Rendereru).
template <typename T, size_t N = 2>
class ZeroAllocStateHistory {
    static_assert(N >= 2, "historie potrebuje aspon dva stavy (prev/curr)");

public:
    ZeroAllocStateHistory() {
        for (size_t i = 0; i < N; ++i) history[i] = T();
    }

    explicit ZeroAllocStateHistory(const T& initial_value) {
        for (size_t i = 0; i < N; ++i) history[i] = initial_value;
    }

    // Zakážeme kopírování/přesouvání držáku historie
    ZeroAllocStateHistory(const ZeroAllocStateHistory&) = delete;
    ZeroAllocStateHistory& operator=(const ZeroAllocStateHistory&) = delete;
    ZeroAllocStateHistory(ZeroAllocStateHistory&&) = delete;
    ZeroAllocStateHistory& operator=(ZeroAllocStateHistory&&) = delete;

    // --- MANIPULACE A ZÁPIS ---

    // Posune ring a vrátí referenci na nový aktuální stav pro zápis
    T& advance_and_get_current() {
        head = (head + 1) % N;
        return history[head];
    }

    void advance_and_load_current(const T& state)
    {
        advance_and_get_current() = state;
    }

    void advance_and_load_current(T&& state)
    {
        advance_and_get_current() = std::move(state);
    }

    // --- PŘÍSTUP PRO ČTENÍ ---

    // stav podle stari: age 0 = nejnovejsi, age 1 = predchozi, ... age N-1 = nejstarsi
    const T& get(size_t age) const {
        return history[(head + N - (age % N)) % N];
    }

    const T& get_current() const {
        return get(0);
    }

    const T& get_previous() const {
        return get(1);
    }

    static constexpr size_t capacity() {
        return N;
    }

private:
    T history[N];
    size_t head = 0; // index nejnovejsiho stavu
};

}
