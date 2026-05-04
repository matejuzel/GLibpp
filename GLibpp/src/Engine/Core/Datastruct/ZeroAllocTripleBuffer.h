#pragma once

#include <atomic>
#include <array>
#include <format>
#include <new> // Kvùli std::hardware_destructive_interference_size

template <typename T>
class ZeroAllocTripleBuffer {
public:
    ZeroAllocTripleBuffer()
        : buffers{}, producer_idx(0), consumer_idx(2) {
        dirty_idx.store(1, std::memory_order_relaxed);
    }

    explicit ZeroAllocTripleBuffer(const T& initial_value)
        : buffers{ initial_value, initial_value, initial_value },
        producer_idx(0), consumer_idx(2) {
        dirty_idx.store(1, std::memory_order_relaxed);
    }

    ZeroAllocTripleBuffer(const ZeroAllocTripleBuffer&) = delete;
    ZeroAllocTripleBuffer& operator=(const ZeroAllocTripleBuffer&) = delete;
    ZeroAllocTripleBuffer(ZeroAllocTripleBuffer&&) = delete;
    ZeroAllocTripleBuffer& operator=(ZeroAllocTripleBuffer&&) = delete;

    // --- PRODUCER API ---
    T& get_write_buffer() {
        return buffers[producer_idx];
    }

    void publish() {
        // Exchange s release sémantikou dává consumeru vìdìt, že data jsou pøipravena
        producer_idx = dirty_idx.exchange(producer_idx, std::memory_order_acq_rel);
    }

    // --- CONSUMER API ---
    bool update_reader() {
        // Rychlý test: Pokud se index nezmìnil, ušetøíme drahý exchange
        if (dirty_idx.load(std::memory_order_relaxed) == consumer_idx) {
            return false;
        }

        // Pokud je dirty_idx jiný, vyzvedneme si ho
        int latest = dirty_idx.exchange(consumer_idx, std::memory_order_acq_rel);

        if (latest == consumer_idx) {
            return false;
        }

        consumer_idx = latest;
        return true;
    }

    const T& get_read_buffer() const {
        return buffers[consumer_idx];
    }

    std::string toString() const {
        return std::format(
            "ZeroAllocTripleBuffer {{ producer_idx: {}, consumer_idx: {}, dirty_idx: {} }}",
            producer_idx, consumer_idx, dirty_idx.load(std::memory_order_relaxed)
        );
    }

private:
    // Detekce a bezpeèné nadefinování velikosti cache line
#ifdef __cpp_lib_hardware_interference_size
    // Pokud kompilátor konstantu podporuje, použijeme ji pøímo
    static constexpr size_t cache_line = std::hardware_destructive_interference_size;
#else
    // Fallback pro systémy/kompilátory, které ji nemají
    static constexpr size_t cache_line = 64;
#endif

    // 1. Samotná data bufferù
    std::array<T, 3> buffers;

    // 2. Data pro Producer vlákno (zarovnáno na novou cache line)
    alignas(cache_line) int producer_idx;

    // 3. Data pro Consumer vlákno (zarovnáno na další cache line)
    alignas(cache_line) int consumer_idx;

    // 4. Sdílená atomická promìnná (zarovnáno na další cache line)
    alignas(cache_line) std::atomic<int> dirty_idx;
};