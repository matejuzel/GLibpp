#include "ConsoleDoubleBuffer.h"

ConsoleDoubleBuffer::ConsoleDoubleBuffer()
{
    h = GetStdHandle(STD_OUTPUT_HANDLE);

    // vypnout kurzor
    CONSOLE_CURSOR_INFO ci;
    GetConsoleCursorInfo(h, &ci);
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(h, &ci);

    // zjistit velikost bufferu
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(h, &csbi);

    width = csbi.dwSize.X;
    height = csbi.dwSize.Y;

    front.assign(height, "");
    back.assign(height, "");
}

void ConsoleDoubleBuffer::clearBack()
{
    for (auto& line : back)
        line.clear();
}

void ConsoleDoubleBuffer::write(int row, const std::string& text)
{
    if (row >= 0 && row < height)
        back[row] = text;
}

void ConsoleDoubleBuffer::present()
{
    for (int y = 0; y < height; y++)
    {
        if (back[y] != front[y])
        {
            COORD pos = { 0, (SHORT)y };
            SetConsoleCursorPosition(h, pos);

            std::string line = back[y];
            if (line.size() < (size_t)width)
                line.append(width - line.size(), ' ');

            std::cout << line;
        }
    }

    front = back;
}
