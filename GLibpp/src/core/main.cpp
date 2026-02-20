#pragma comment(lib, "User32.lib")

#define USE_WINAPI

#include <iostream>
#include "App.h"

using namespace std;

#ifdef USE_WINAPI
#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:

        int key = 0;
        App::instance().onKeydown((int) wParam);

        if (wParam == VK_ESCAPE) {

            PostQuitMessage(0);
        }

        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif


int main()
{

    App::instance().work();



#ifdef USE_WINAPI

    // ---------------------------------------------------------
    // 2) WINAPI VERZE – otevře prázdné okno
    // ---------------------------------------------------------

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    //const char CLASS_NAME[] = "MyWinAPIWindowClass";
    const wchar_t CLASS_NAME[] = L"MyWinAPIWindowClass";


    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Moje WinAPI okno",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd == nullptr)
        return 1;

    ShowWindow(hwnd, SW_SHOW);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;

#endif
}
