#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <syncstream>

//#include "core/Types.h"
//#include "core/App.h"
#include "window/WindowWin32.h"
#include "window/WindowProcedure.h"
#include "window/DIB/DIBFramebuffer.h"
#include "window/DIB/DIBRenderer.h"
//#include "demo/ProducentConsumentDemo.h"
//#include "demo/DemoRunner.h"
//#include "core/render/RenderContext.h"

#pragma comment(lib, "User32.lib")

int main()
{

    WindowWin32 win(1000,800, WindowProc);
    win.build();
    //win.resizeWindowToFillScreen();

    DIBFramebuffer fb(1000, 800, 32);
    fb.init(win.getHwnd());
    
    DIBRenderer rend(fb);

    rend.clear(0x00000000);
    rend.drawCircle(20, 20, 10, 0x00ff0000);
    
    fb.present();

    MSG msg = {};
    bool running = true;

    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;

}


