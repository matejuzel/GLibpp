#pragma once

#include <windows.h>
#include "core/input/Keymap.h"

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
        if (size == 0) break;

        std::vector<BYTE> buffer(size);
        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == (UINT)-1)
            break;

        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());

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

    }
    break;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
