#pragma once
#include <chrono>
#include <deque>

class Fps {
public:
    explicit Fps(double windowSeconds = 1.0);

    void tick();
    double getFps() const;
    double getLowPercentile(double p) const; // p = 0.99 -> 1% low

    double getLow1Percent() const { return getLowPercentile(0.99); }
    double getLowPoint1Percent() const { return getLowPercentile(0.999); }

private:
    std::deque<std::chrono::steady_clock::time_point> timestamps;
    std::chrono::duration<double> window;
};
