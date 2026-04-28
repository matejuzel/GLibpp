#pragma once

class RunState {
    std::atomic<bool> m_running{ false };

public:
    // Explicitnì zakážeme kopírování, protože std::atomic není kopírovatelný
    RunState() = default;
    RunState(const RunState&) = delete;
    RunState& operator=(const RunState&) = delete;

    void start() noexcept {
        m_running.store(true, std::memory_order_relaxed);
    }

    void stop() noexcept {
        m_running.store(false, std::memory_order_relaxed);
    }

    operator bool() const noexcept {
        return m_running.load(std::memory_order_relaxed);
    }

    bool isRunning() const noexcept {
        return m_running.load(std::memory_order_relaxed);
    }
};
