#pragma once

class FixedTimestep
{
public:

    const double dt;

    FixedTimestep(double hz) : dt(1.0 / hz) {}

    // Vrací poèet logických krokù, které je potøeba provést
    int consume(double frameTime);

    double getDt() const { return dt; }

private:

    double accumulator = 0.0;
};
