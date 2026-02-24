#include "HighResTimer.h"
#include <chrono>
#include <windows.h>

// vrátí èas od posledního volání v sekundách
double HighResTimer::tick()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    double dt = double(now.QuadPart - prev.QuadPart) / double(freq.QuadPart);
    prev = now;
    return dt;
}

// vrátí èas od posledního resetu
double HighResTimer::elapsed() const
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return double(now.QuadPart - start.QuadPart) / double(freq.QuadPart);
}

void HighResTimer::reset()
{
    QueryPerformanceCounter(&start);
    prev = start;
}
