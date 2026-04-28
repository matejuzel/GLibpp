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

    bool consumeAll(double frameTime)
    {
        // Nejdøíve pøièteme uplynulý èas od posledního snímku
        accumulator += frameTime;

        if (accumulator >= dt)
        {
            // std::fmod je správná volba pro double - ponechá v akumulátoru 
            // jen to, co se "nevešlo" do celistvých dt krokù.
            accumulator = std::fmod(accumulator, dt);
            return true;
        }
        return false;
    }

    int consumeAllSteps(double frameTime) // Pøidej parametr!
    {
        accumulator += frameTime; // Pøièti èas!
        int steps = static_cast<int>(accumulator / dt);
        accumulator -= steps * dt;
        return steps;
    }

    double getDt() const { return dt; }

private:

    double accumulator = 0.0;
};
