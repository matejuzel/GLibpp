#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <syncstream>

#include "core/Types.h"
#include "core/App.h"
#include "window/WindowBuilder.h"
#include "demo/ProducentConsumentDemo.h"
#include "window/WindowProcedure.h"
#include "demo/DemoRunner.h"
#include "core/render/RenderContext.h"

#pragma comment(lib, "User32.lib")

int main()
{
    int run = 0;

    switch (run) {
        case 1: DemoRunner::renderLoopAndDie(); break;
        case 2: DemoRunner::producentConsumentAndDie(); break;

        case 0:
        default:
            break; // default app run
    }

    // Application entry point
    int width = 1200;
    int height = 900;

    WindowBuilder wnd(width, height, WindowProc);
    if (!wnd.build())
    {
        std::cerr << "Window build failed" << std::endl;
        return 1;
    }

    if (!wnd.DIB_init())
    {
        std::cerr << "DIB_init failed" << std::endl;
        return 1;
    }

    Renderer renderer(&wnd);

    App::instance().renderer = &renderer;
    App::instance().init();
    App::instance().runGameLoop();

    return 0;
}


