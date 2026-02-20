#pragma comment(lib, "User32.lib")

#define USE_WINAPI

#include <iostream>
#include "core/App.h"
#include "core/input/KeyCode.h"


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
        KeyCode key = KeyCode::Unknown;

        switch (wParam)
        {
        case VK_LEFT:   key = KeyCode::Left; break;
        case VK_RIGHT:  key = KeyCode::Right; break;
        case VK_UP:     key = KeyCode::Up; break;
        case VK_DOWN:   key = KeyCode::Down; break;

        case VK_ESCAPE: key = KeyCode::Esc; break;
        case VK_SPACE:  key = KeyCode::Space; break;
        case VK_RETURN: key = KeyCode::Enter; break;
        case VK_SHIFT:  key = KeyCode::Shift; break;
        case VK_CONTROL:key = KeyCode::Ctrl; break;
        }

        App::instance().onKeydown(key);
    break;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#endif

using namespace std;


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
