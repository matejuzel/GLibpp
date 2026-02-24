#include <windows.h>
#include <iostream>
#include <chrono>
#include "core/App.h"
#include "core/input/Keyboard.h"
#include "core/input/Keymap.h"
#include "window/WindowBuilder.h"

#pragma comment(lib, "User32.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
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


bool mainLoop()
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

        app.update();
        app.render();

        Sleep(10);
    }

    return true;
}

int main()
{
    WindowBuilder wnd(mainLoop, WindowProc);

    if (!wnd.init())
    {
        std::cout << "chyba pri vytvoreni WIN onka" << std::endl;
        return 1;
    }

    wnd.run();

    return 0;
}


