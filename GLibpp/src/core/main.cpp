#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
//#include <iostream>
#include <chrono>
#include <thread>
#include <syncstream>
#include <thread>

#include "window/WindowWin32.h"
#include "window/WindowProcedure.h"
#include "window/DIB/DIBFramebuffer.h"
#include "window/DIB/DIBRenderer.h"

#include "core/EngineApp.h"

#pragma comment(lib, "User32.lib")

void aaa() {
    const uint32_t width = 400;
    const uint32_t height = 320;

    WindowWin32 win(width, height, WindowProc, false);
    win.build();

    DIBFramebuffer fb(width, height, 32);
    fb.init(win.getHwnd());

    DIBRenderer rend(fb);

    rend.clear(0x00000000);
    rend.drawCircle(20, 20, 10, 0x00ff0000);
    rend.drawCircle(20, 20, 6, 0x000000);

    fb.present();

    MSG msg = {};
    bool running = true;

    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int main()
{

    EngineApp app;

    app.init();
    app.runLoop();


    /*
    const uint32_t width = 400;
    const uint32_t height = 320;

    WindowWin32 win(width, height, WindowProc, false);
    win.build();

    DIBFramebuffer fb(width, height, 32);
    fb.init(win.getHwnd());
    
    DIBRenderer rend(fb);

    rend.clear(0x00000000);
    rend.drawCircle(20, 20, 10, 0x00ff0000);
    rend.drawCircle(20, 20, 6, 0x00ffffff);
    
    fb.present();

    MSG msg = {};
    bool running = true;

    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    
    */
    return 0;

}


