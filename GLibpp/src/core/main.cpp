#include <windows.h>
#include <iostream>
#include <chrono>
#include "core/App.h"
#include "core/input/Keyboard.h"
#include "core/input/Keymap.h"
#include "window/WindowBuilder.h"

#include "utils/timer/HighResTimer.h"
#include "utils/timer/FixedTimestep.h"
#include <thread>
#include <syncstream>
#include "utils/datastruct/SPSCQueue.h"

#pragma comment(lib, "User32.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:

        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;

    case WM_INPUT:
    {
        UINT size = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

        BYTE* buffer = new BYTE[size];
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER));

        RAWINPUT* raw = (RAWINPUT*)buffer;

        if (raw->header.dwType == RIM_TYPEKEYBOARD)
        {
            USHORT vk = raw->data.keyboard.VKey;
            USHORT flags = raw->data.keyboard.Flags;

            bool isUp = (flags & RI_KEY_BREAK) != 0;

            KeyMap key = KEYMAP[vk];

            if (isUp)
                App::instance().keyboard.keyUp((size_t)key);
            else
                App::instance().keyboard.keyDown((size_t)key);
        }

        delete[] buffer;
    }
    break;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


bool mainLoopFixedTimestamp(float logicHz = 60.0f)
{
    App& app = App::instance();
    app.init();

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

        // čas
        double frameTime = timer.tick();
        if (frameTime > 0.25) frameTime = 0.25;

        int steps;
        
        steps = taskLogic.consume(frameTime);
        for (int i = 0; i < steps; i++) {
            app.update((float)taskLogic.getDt());
        }

        steps = taskCmd.consume(frameTime);
        for (int i = 0; i < steps; i++) {

            float fps = (float) (frames / timer.sinceStart());

            app.__cmdUpdate((float)taskCmd.getDt(), fps);
        }

        // render
        app.render();
        frames++;
    }

    return true;
}

bool mainLoopBasic(float dummy)
{

    App& app = App::instance();
    app.init();

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




SPSCQueue<int, 256> queue;

void producer() {

    for (int i = 0; i < 100000; i++) {

        queue.pushLoop(i);
        std::cout << "Producer:" << i << std::endl;

        //if (i % 10 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //if (i % 100 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    queue.pushLoop(-1);
}

void consumer() {

    while (true) {

        int value;
        queue.popLoop(value);
        if (value == -1) return;
        std::cout << "Consumer: " << value << std::endl;
    }
}

int main()
{
    std::thread t1(producer);
    std::thread t2(consumer);

    t1.join();
    t2.join();

    return 0;











    WindowBuilder wnd(mainLoopFixedTimestamp, WindowProc);
    //WindowBuilder wnd(mainLoopBasic, WindowProc);



    if (!wnd.init(1200, 800))
    {
        std::cout << "chyba pri vytvoreni WIN onka" << std::endl;
        return 1;
    }

    wnd.DIB_init();
    App::instance().setWindow(&wnd);
    wnd.run(60.0f);

    return 0;
}


