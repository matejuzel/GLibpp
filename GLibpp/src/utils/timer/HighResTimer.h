#pragma once

#include <chrono>
#include <windows.h>

class HighResTimer
{
public:
    HighResTimer();

    // vrátí èas od posledního volání v sekundách
    double tick();

    // vrátí èas od posledního resetu
    double elapsed() const;

    void reset();

    double sinceStart() const;

    double nowSeconds() const;

private:
    LARGE_INTEGER freq;
    LARGE_INTEGER prev;
    LARGE_INTEGER start;
    LARGE_INTEGER absoluteStart;
};
