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
//#include "demo/ProducentConsumentDemo.h"
#include "window/WindowProcedure.h"
//#include "demo/DemoRunner.h"
//#include "core/render/RenderContext.h"

#pragma comment(lib, "User32.lib")

int main()
{

    auto& app = App::instance();
    app.init();



    return 0;







    //DemoRunner::renderLoopAndDie();
    //DemoRunner::producentConsumentAndDie();

    /*
    int width = 1600;
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
;
    App::instance().renderer = &renderer;
    App::instance().init();
    App::instance().runGameLoop();

    return 0;
    */
}


