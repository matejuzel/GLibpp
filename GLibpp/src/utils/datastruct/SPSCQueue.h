#pragma once

#include <atomic>
#include "utils/cpu_pause.h"

template<typename T, size_t N>
class SPSCQueue {
public:
    bool push(const T& v);
    bool pop(T& out);

    void pushLoop(const T& v);
    void popLoop(T& v);

private:
    T buffer_[N];
    std::atomic<size_t> head_{ 0 };
    std::atomic<size_t> tail_{ 0 };
};

template<typename T, size_t N>
bool SPSCQueue<T, N>::push(const T& v)
{
    size_t head = head_.load(std::memory_order_relaxed);
    size_t next = (head + 1) % N;

    if (next == tail_.load(std::memory_order_acquire))
        return false; // plné

    buffer_[head] = v;
    head_.store(next, std::memory_order_release);
    return true;
}

template<typename T, size_t N>
bool SPSCQueue<T, N>::pop(T& out)
{
    size_t tail = tail_.load(std::memory_order_relaxed);

    if (tail == head_.load(std::memory_order_acquire))
        return false; // prázdné

    out = buffer_[tail];
    tail_.store((tail + 1) % N, std::memory_order_release);
    return true;
}

template<typename T, size_t N>
void SPSCQueue<T, N>::pushLoop(const T& v) 
{
    while (!this->push(v))
    {
        CPU_PAUSE();
    }
}

template<typename T, size_t N>
void SPSCQueue<T, N>::popLoop(T& v) 
{
    while (!this->pop(v)) 
    {
        CPU_PAUSE();
    }
}

