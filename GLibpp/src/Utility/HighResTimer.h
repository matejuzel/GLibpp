#pragma once

#include <chrono>
#include <windows.h>

class HighResTimer
{
public:
    HighResTimer()
    {
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&prev);
        QueryPerformanceCounter(&absoluteStart);
        start = absoluteStart;

    }

    // vrátí èas od posledního volání v sekundách
    double tick()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        double dt = double(now.QuadPart - prev.QuadPart) / double(freq.QuadPart);
        prev = now;
        return dt;
    }

    // vrátí èas od posledního resetu
    double elapsed() const
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return double(now.QuadPart - start.QuadPart) / double(freq.QuadPart);
    }

    void reset()
    {
        QueryPerformanceCounter(&start);
        prev = start;
    }

    double sinceStart() const
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return double(now.QuadPart - absoluteStart.QuadPart) / double(freq.QuadPart);
    }

    double nowSeconds() const
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return double(now.QuadPart) / double(freq.QuadPart);
    }

private:
    LARGE_INTEGER freq;
    LARGE_INTEGER prev;
    LARGE_INTEGER start;
    LARGE_INTEGER absoluteStart;
};
