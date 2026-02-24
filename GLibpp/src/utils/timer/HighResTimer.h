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
    }

    // vrátí èas od posledního volání v sekundách
    double tick();

    // vrátí èas od posledního resetu
    double elapsed() const;

    void reset();

private:
    LARGE_INTEGER freq;
    LARGE_INTEGER prev;
    LARGE_INTEGER start;
};
