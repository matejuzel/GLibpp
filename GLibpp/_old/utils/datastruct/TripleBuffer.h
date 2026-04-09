#pragma once
#include <atomic>
#include <array>
#include <vector>

template<typename T>
class TripleBuffer
{
public:
    TripleBuffer() = default;

    explicit TripleBuffer(const T& initial)
    {
        buffers_[0] = initial;
        buffers_[1] = initial;
        buffers_[2] = initial;
    }

    // Producer: buffer pro zápis
    T& writeBuffer() noexcept
    {
        const int r = readIndex_.load(std::memory_order_acquire);
        const int w = writeIndex_.load(std::memory_order_relaxed);

        // Pokud by write == read, posuneme write na tøetí buffer
        if (w == r)
            writeIndex_.store((r + 1) % 3, std::memory_order_relaxed);

        return buffers_[writeIndex_.load(std::memory_order_relaxed)];
    }

    // Producer: po dopsání zavolá swap
    void publish() noexcept
    {
        const int w = writeIndex_.load(std::memory_order_relaxed);
        readIndex_.store(w, std::memory_order_release);

        // Najdeme nový write buffer (ten, který není read)
        const int r = w;
        const int next = (r + 1) % 3;
        writeIndex_.store(next, std::memory_order_relaxed);
    }

    // Consumer: buffer pro ètení
    const T& readBuffer() const noexcept
    {
        const int r = readIndex_.load(std::memory_order_acquire);
        return buffers_[r];
    }

private:
    std::array<T, 3> buffers_{};
    std::atomic<int> readIndex_{ 0 };
    std::atomic<int> writeIndex_{ 1 };
};
