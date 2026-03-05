#include "utils/timer/Fps.h"
#include <algorithm>
#include <vector>

Fps::Fps(double windowSeconds)
    : window(windowSeconds)
{
}

void Fps::tick() {
    using clock = std::chrono::steady_clock;
    const auto now = clock::now();

    timestamps.push_back(now);

    const auto cutoff = now - window;
    while (!timestamps.empty() && timestamps.front() < cutoff) {
        timestamps.pop_front();
    }
}

double Fps::getFps() const {
    return static_cast<double>(timestamps.size()) / window.count();
}

double Fps::getLowPercentile(double p) const {
    if (timestamps.size() < 2) return 0.0;

    std::vector<double> frametimes;
    frametimes.reserve(timestamps.size() - 1);

    for (size_t i = 1; i < timestamps.size(); ++i) {
        auto dt = std::chrono::duration<double>(timestamps[i] - timestamps[i - 1]).count();
        frametimes.push_back(dt);
    }

    std::sort(frametimes.begin(), frametimes.end()); // vzestupnì

    size_t idx = static_cast<size_t>(p * (frametimes.size() - 1));
    double worstFrameTime = frametimes[idx];

    return 1.0 / worstFrameTime;
}
