#pragma once

#include <array>

template<typename T>
class DoubleBuffer {
public:
    DoubleBuffer() {
        index_.store(0, std::memory_order_relaxed);
    }

    // reference na back buffer (zapis)
    T& writeBuffer() noexcept {
        return buffers_[backIndex()];
    }

    // const reference na front buffer (cteni)
    const T& readBuffer() const noexcept {
        return buffers_[frontIndex()];
    }

    // atomicky prehodit back -> front
    void publish() noexcept {
        // increment index mod 2
        int prev = index_.load(std::memory_order_relaxed);
        int next = (prev + 1) & 1;
        index_.store(next, std::memory_order_release);
    }

    // vrati index front/back (pomocne pro integraci s existujicim API)
    int frontIndex() const noexcept {
        return index_.load(std::memory_order_acquire);
    }

    int backIndex() const noexcept {
        return (frontIndex() + 1) & 1;
    }

private:
    std::array<T, 2> buffers_;
    std::atomic<int> index_; // index front bufferu (0 nebo 1)
};
