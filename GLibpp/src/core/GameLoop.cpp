#include "core/GameLoop.h"
#include "core/App.h"
#include "utils/timer/HighResTimer.h"
#include "utils/timer/FixedTimestep.h"

bool GameLoop::mainLoopFixedTimestamp()
{
    App& app = App::instance();
    app.init(1200, 800);

    MSG msg = {};

    HighResTimer timer;
    FixedTimestep taskLogic(60.0);
    FixedTimestep taskCmd(60.0);

    int frames = 0;

    while (true)
    {
        // messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // èas
        double frameTime = timer.tick();
        if (frameTime > 0.25) frameTime = 0.25;

        int steps;

        steps = taskLogic.consume(frameTime);
        for (int i = 0; i < steps; i++) {
            app.update((float)taskLogic.getDt());
        }

        steps = taskCmd.consume(frameTime);
        for (int i = 0; i < steps; i++) {

            float fps = (float)(frames / timer.sinceStart());

            app.__cmdUpdate((float)taskCmd.getDt(), fps);
        }

        // render
        app.render();
        frames++;
    }

    return true;
}

bool GameLoop::mainLoopBasic()
{
    App& app = App::instance();
    app.init(1200, 800);

    MSG msg = {};
    while (true)
    {

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        app.update(0.01f);
        app.render();

        Sleep(10);
    }

    return true;
}
