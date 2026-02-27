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

    WindowBuilder wnd(1200, 800, WindowProc);

    if (!wnd.build())
    {
        std::cout << "chyba pri vytvoreni WIN onka" << std::endl;
        return 1;
    }

    wnd.DIB_init();
    App::instance().setWindow(&wnd);
    App::instance().runGameLoop();

    return 0;
}


