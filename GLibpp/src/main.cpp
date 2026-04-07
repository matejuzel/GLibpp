#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
//#include <iostream>
#include <chrono>
#include <thread>
#include <syncstream>
#include <thread>

#include "branch.h"

//#include "window/WindowWin32.h"
//#include "window/DIB/DIBFramebuffer.h"
//#include "window/DIB/DIBRenderer.h"
//#include "core/EngineApp.h"

//#pragma comment(lib, "User32.lib")


int main()
{
    EngineApp app;
    app.runRenderingLoop();


    //EngineApp app;
    //app.init();
    //app.runLoop();

    return 0;
}


