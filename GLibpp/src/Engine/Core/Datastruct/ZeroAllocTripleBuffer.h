#pragma once

#include <atomic>
#include <array>

template <typename T>
class ZeroAllocTripleBuffer {
public:
    // Konstruktor alokuje/inicializuje všechny 3 buffery hned na zaèátku
    ZeroAllocTripleBuffer() {
        // Buffery se vytvoøí pomocí výchozího konstruktoru typu T
        buffers[0] = T();
        buffers[1] = T();
        buffers[2] = T();

        producer_idx = 0;
        consumer_idx = 2;
        dirty_idx.store(1, std::memory_order_relaxed);
    }

    // Konstruktor s výchozí hodnotou pro inicializaci všech tøí bufferù
    explicit ZeroAllocTripleBuffer(const T& initial_value) {
        buffers[0] = initial_value;
        buffers[1] = initial_value;
        buffers[2] = initial_value;

        producer_idx = 0;
        consumer_idx = 2;
        dirty_idx.store(1, std::memory_order_relaxed);
    }

    // Zakážeme kopírování a pøesun samotného bufferu pro bezpeèné použití ve více vláknech
    ZeroAllocTripleBuffer(const ZeroAllocTripleBuffer&) = delete;
    ZeroAllocTripleBuffer& operator=(const ZeroAllocTripleBuffer&) = delete;
    ZeroAllocTripleBuffer(ZeroAllocTripleBuffer&&) = delete;
    ZeroAllocTripleBuffer& operator=(ZeroAllocTripleBuffer&&) = delete;

    // --- PRODUCER (Logic Thread) API ---

    // Získání pøímé reference na buffer, do kterého se má zapisovat.
    // Žádná alokace ani kopírování se nekoná.
    T& get_write_buffer() {
        return buffers[producer_idx];
    }

    // Zveøejnìní (publikace) novì zapsaných dat.
    // Prohodí index aktuálního zápisového bufferu s indexem 'dirty'.
    void publish() {
        producer_idx = dirty_idx.exchange(producer_idx, std::memory_order_acq_rel);
    }

    // --- CONSUMER (Render Thread) API ---

    // Aktualizace ètecího bufferu. Pokud jsou k dispozici nová data,
    // prohodí se indexy a vrátí true. Jinak vrátí false.
    bool update_reader() {
        int latest = dirty_idx.exchange(consumer_idx, std::memory_order_acq_rel);

        if (latest == consumer_idx) {
            // Žádná nová data od posledního ètení
            return false;
        }

        consumer_idx = latest;
        return true;
    }

    // Získání reference na aktuální ètecí buffer pro Render thread
    const T& get_read_buffer() const {
        return buffers[consumer_idx];
    }

private:
    std::array<T, 3> buffers;

    int producer_idx; // Pouze pro vlákno zapisující
    int consumer_idx; // Pouze pro vlákno ètoucí

    std::atomic<int> dirty_idx; // Atomický prostøedník
};