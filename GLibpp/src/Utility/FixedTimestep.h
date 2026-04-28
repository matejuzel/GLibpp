#pragma once

class FixedTimestep
{
public:

    const double dt;

    FixedTimestep(double hz) : dt(1.0 / hz) {}

    // Vrací poèet logických krokù, které je potøeba provést
    int consume(double frameTime) 
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

    double getDt() const { return dt; }

private:

    double accumulator = 0.0;
};
