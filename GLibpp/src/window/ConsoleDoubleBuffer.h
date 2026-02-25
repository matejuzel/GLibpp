
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <iostream>

class ConsoleDoubleBuffer
{
public:
    ConsoleDoubleBuffer();

    void clearBack();

    void write(int row, const std::string& text);

    void present();

private:
    HANDLE h;
    int width, height;
    std::vector<std::string> front;
    std::vector<std::string> back;
};
