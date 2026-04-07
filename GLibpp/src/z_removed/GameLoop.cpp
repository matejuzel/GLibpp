#include "core/GameLoop.h"
#include "utils/timer/HighResTimer.h"
#include "utils/timer/FixedTimestep.h"
#include <RenderCommand.h>
#include <Renderer.h>
#include <thread>


bool GameLoop::mainLoopFixedTimestepBufferedAndQueueInterpolated()
{
    App& app = App::instance();

    auto& renderer = *app.renderer;

    std::thread t_renderLoop(&Renderer::runRenderLoop, &renderer);

    MSG msg = {};

    const double dt = 1.0 / 60.0;
    double accumulatorLogic = 0.0;
    double accumulatorCmd = 0.0;

    HighResTimer timer;
    timer.tick(); // reset

    while (renderer.isRunning())
    {
        // Windows messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                renderer.stop();
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // èas
        double frameTime = timer.tick();
        if (frameTime > 0.25) frameTime = 0.25;

        accumulatorLogic += frameTime;
        accumulatorCmd += frameTime;

        // --- LOGIC UPDATE ---
        while (accumulatorLogic >= dt)
        {
            app.update((float)dt);
            accumulatorLogic -= dt;
        }

        // --- COMMAND UPDATE ---
        while (accumulatorCmd >= dt)
        {
            app.__cmdUpdate((float)dt);
            accumulatorCmd -= dt;
        }

        // --- INTERPOLACE PRO RENDER ---
        float alpha = (float)(accumulatorLogic / dt);

        // bezpeèné pøedání interpolace rendereru
        // @todo
        //renderer.setInterpolationAlpha(alpha);
    }

    renderer.stop();
    t_renderLoop.join();

    return true;
}


bool GameLoop::mainLoopFixedTimestepBufferedAndQueue() 
{

    App& app = App::instance();
    auto& renderer = *app.renderer;
    
    std::thread t_renderLoop(&Renderer::runRenderLoop, &renderer);

    MSG msg = {};

    HighResTimer timer;
    FixedTimestep taskLogic(60.0);
    FixedTimestep taskCmd(60.0);
    
    while (renderer.isRunning())
    {
        // messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
				renderer.stop();
                break;
            };
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

            app.__cmdUpdate((float)taskCmd.getDt());
        }
    }

	renderer.stop();
    t_renderLoop.join();

    return true;
}

bool GameLoop::mainLoopFixedTimestamp()
{
    App& app = App::instance();

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

            app.__cmdUpdate((float)taskCmd.getDt());
        }

        //app.renderer->drawScene();

        frames++;
    }

    return true;
}

bool GameLoop::mainLoopBasic()
{
    App& app = App::instance();

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
        
		//app.renderer->drawScene();

        Sleep(10);
    }

    return true;
}
