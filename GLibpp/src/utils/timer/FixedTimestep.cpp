#include "FixedTimestep.h"

// Vrací poèet logických krokù, které je potøeba provést
int FixedTimestep::consume(double frameTime)
{
    accumulator += frameTime;

    int steps = 0;
    while (accumulator >= dt)
    {
        accumulator -= dt;
        steps++;
    }

    return steps;
}
