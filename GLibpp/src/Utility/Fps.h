#pragma once
#include <chrono>
#include <deque>

class Fps {
public:
    explicit Fps(double windowSeconds)
        : window(windowSeconds)
    {}

    void tick()
    {
        using clock = std::chrono::steady_clock;
        const auto now = clock::now();

        timestamps.push_back(now);

        const auto cutoff = now - window;
        while (!timestamps.empty() && timestamps.front() < cutoff) {
            timestamps.pop_front();
        }
    }

    double getFps() const
    {
        return static_cast<double>(timestamps.size()) / window.count();
    }

    double getLowPercentile(double p) const // p = 0.99 -> 1% low
    {
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

    double getLow1Percent() const { return getLowPercentile(0.99); }
    double getLowPoint1Percent() const { return getLowPercentile(0.999); }

private:
    std::deque<std::chrono::steady_clock::time_point> timestamps;
    std::chrono::duration<double> window;
};
