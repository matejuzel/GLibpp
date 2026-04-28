#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "App.h"

//#pragma comment(lib, "User32.lib")

int main()
{

    try 
    {
        App app;
		app.initialize(150, 80);
        app.run();
    }
    catch (std::runtime_error error) 
    { 
        std::cout << error.what() << std::endl; 
        return 1; 
    }

    return 0;
}


