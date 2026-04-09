#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "App.h"

//#pragma comment(lib, "User32.lib")

int main()
{
    App app;
    app.runRenderingLoop();

    return 0;
}


