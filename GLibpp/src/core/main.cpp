#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <syncstream>

#include "core/App.h"
#include "window/WindowBuilder.h"
#include "utils/datastruct/SPSCQueue.h"
#include "demo/ProducentConsumentDemo.h"
#include "window/WindowProcedure.h"

#include "demo/DemoRunner.h"

#pragma comment(lib, "User32.lib")


int main()
{
    //DemoRunner::producentConsumentAndDie();

    auto& app = App::instance();

    WindowBuilder wnd(2000, 800, WindowProc);

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

    app.setWindow(&wnd);
    app.runGameLoop();

    return 0;
}


