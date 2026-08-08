#pragma once

#include <atomic>

namespace GLibpp::Core {

// Stavovy latch bezici smycky: stop() je terminalni, start() po stop()
// uz stav neozivi. Resi zavod start/stop mezi vlakny - kdyz stop()
// (ESC/WM_CLOSE) predbehne start() render smycky (freeze + upload walk
// trva v Debugu desitky ms), smycka se uz nerozbehne; drivejsi bool
// varianta by stop prepsala a join() by visel navzdy.
class RunState {

    enum State : int { NotStarted = 0, Running = 1, Stopped = 2 };

    std::atomic<int> m_state{ NotStarted };

public:
    // std::atomic neni kopirovatelny - kopie zakazane explicitne
    RunState() = default;
    RunState(const RunState&) = delete;
    RunState& operator=(const RunState&) = delete;

    // prepne jen NotStarted -> Running; po stop() je no-op (latch)
    void start() noexcept {
        int expected = NotStarted;
        m_state.compare_exchange_strong(expected, Running,
            std::memory_order_acq_rel, std::memory_order_acquire);
    }

    // terminalni - zadny nasledny start() uz stav neprepne zpet
    void stop() noexcept {
        m_state.store(Stopped, std::memory_order_release);
    }

    operator bool() const noexcept {
        return isRunning();
    }

    bool isRunning() const noexcept {
        return m_state.load(std::memory_order_acquire) == Running;
    }
};

}
