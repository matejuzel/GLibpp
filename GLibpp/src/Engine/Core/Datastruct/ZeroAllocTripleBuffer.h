#pragma once

#include <atomic>
#include <array>
#include <format>
#include <new>

template <typename T>
class ZeroAllocTripleBuffer {
public:
    ZeroAllocTripleBuffer()
        : producer_idx(0) // Producer má buffer 0
        , consumer_idx(2) // Consumer má buffer 2
    {
        // V "dirty" schránce leží buffer 1. 
        // Schránka teï není "prázdná", ale drží ten tøetí volný buffer.
        dirty_idx.store(1, std::memory_order_relaxed);
        has_new_data.store(false, std::memory_order_relaxed);
    }

    // --- PRODUCER API ---
    T& get_write_buffer() {
        return buffers[producer_idx];
    }

    void publish() {
        // Vymìníme náš dopsaný buffer za ten, který byl v dirty_idx (ten volný).
        producer_idx = dirty_idx.exchange(producer_idx, std::memory_order_acq_rel);

        // Signalizujeme, že v dirty_idx je teï èerstvé maso.
        has_new_data.store(true, std::memory_order_release);
    }

    // --- CONSUMER API ---
    bool update_reader() {
        // Nejdøív levnì zkontrolujeme, jestli producent vùbec nìco poslal.
        if (!has_new_data.load(std::memory_order_acquire)) {
            return false;
        }

        // Pokud ano, vezmeme si to a v dirty_idx necháme náš starý (už pøeètený) buffer.
        consumer_idx = dirty_idx.exchange(consumer_idx, std::memory_order_acq_rel);

        // Resetujeme pøíznak, protože jsme právì vybrali nejnovìjší data.
        has_new_data.store(false, std::memory_order_release);

        return true;
    }

    const T& get_read_buffer() const {
        return buffers[consumer_idx];
    }

private:
#ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t cache_line = std::hardware_destructive_interference_size;
#else
    static constexpr size_t cache_line = 64;
#endif

    std::array<T, 3> buffers;

    alignas(cache_line) int producer_idx;
    alignas(cache_line) int consumer_idx;

    // dirty_idx teï slouží jako permanentní úschovna toho "tøetího" bufferu.
    alignas(cache_line) std::atomic<int> dirty_idx;

    // Pøíznak, abychom poznali, jestli to, co leží v dirty_idx, 
    // je novinka od producenta, nebo jen starý odložený buffer.
    alignas(cache_line) std::atomic<bool> has_new_data;
};