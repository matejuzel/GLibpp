#pragma once

#include <atomic>
#include <array>
#include <new>

namespace GLibpp::Core {

template <typename T>
class ZeroAllocTripleBuffer {
public:
    ZeroAllocTripleBuffer()
        : producer_idx(0) // Producer má buffer 0
        , consumer_idx(2) // Consumer má buffer 2
    {
        // V "dirty" schránce leží třetí volný buffer (1) - bez FRESH bitu,
        // takže consumer před prvním publish() nic nevyzvedne.
        dirty_idx.store(1, std::memory_order_relaxed);
    }

    // --- PRODUCER API ---
    T& get_write_buffer() {
        return buffers[producer_idx];
    }

    void publish() {
        // Dopsaný buffer odložíme do dirty schránky označený jako čerstvý
        // a vezmeme si ten, který tam ležel (starý odložený, případně čerstvý
        // nepřečtený - ten se tím korektně zahodí, consumer bere jen nejnovější).
        // Index i příznak čerstvosti cestují jedním exchange - nemůže nastat
        // stav "čerstvý buffer bez příznaku" (dřívější oddělený flag uměl
        // v úzkém okně tiše zahodit jeden publish).
        producer_idx = stripFresh(dirty_idx.exchange(withFresh(producer_idx), std::memory_order_acq_rel));
    }

    // --- CONSUMER API ---
    bool update_reader() {
        // Levná kontrola bez RMW: leží v dirty schránce nepřečtený stav?
        if (!isFresh(dirty_idx.load(std::memory_order_acquire))) {
            return false;
        }

        // Vyměníme náš starý (už přečtený) buffer za čerstvý; odkládáme
        // bez FRESH bitu - příznak zaniká přesně v okamžiku vyzvednutí.
        consumer_idx = stripFresh(dirty_idx.exchange(consumer_idx, std::memory_order_acq_rel));

        return true;
    }

    const T& get_read_buffer() const {
        return buffers[consumer_idx];
    }

private:
    // Příznak čerstvosti pakovaný přímo do dirty_idx: indexy 0-2 zabírají
    // bity 0-1, FRESH sedí v bitu 2. Jeden atomik = žádná ztracená notifikace.
    static constexpr int FRESH_BIT = 0b100;

    static constexpr int  withFresh(int idx) noexcept { return idx | FRESH_BIT; }
    static constexpr int  stripFresh(int v) noexcept { return v & ~FRESH_BIT; }
    static constexpr bool isFresh(int v) noexcept { return (v & FRESH_BIT) != 0; }

#ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t cache_line = std::hardware_destructive_interference_size;
#else
    static constexpr size_t cache_line = 64;
#endif

    std::array<T, 3> buffers;

    alignas(cache_line) int producer_idx;
    alignas(cache_line) int consumer_idx;

    // dirty_idx slouží jako permanentní úschovna toho "třetího" bufferu
    // + nese FRESH bit (je to novinka od producenta, nebo odložený starý?).
    alignas(cache_line) std::atomic<int> dirty_idx;
};

}
