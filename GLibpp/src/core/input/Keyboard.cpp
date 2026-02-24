#include <iostream>
#include <string>
#include <windows.h>
#include "core/input/Keyboard.h"
#include "core/input/KeyMap.h"

// keyNames indexované podle KeyMap enumu
const char* Keyboard::keyNames[Keyboard::length] =
{
    "Unknown", // KEY_UNKNOWN = 0
    "Left",    // KEY_LEFT
    "Right",   // KEY_RIGHT
    "Up",      // KEY_UP
    "Down",    // KEY_DOWN
    "Esc",     // KEY_ESC
    "Space",   // KEY_SPACE
    "Enter",   // KEY_ENTER
    "Shift",   // KEY_SHIFT
    "Ctrl"     // KEY_CTRL
    // ... zbytek je automaticky "Unknown"
};


Keyboard::Keyboard() {}


void Keyboard::keyDown(size_t code)
{
    if (code >= length) return;
    this->keys[code] = true;

    std::cout << "down(" << code << ") [" << (*this)[code] << "]\n";
    std::cout << this->toString() << "\n";
}

void Keyboard::keyUp(size_t code)
{
    if (code >= length) return;
    this->keys[code] = false;
}

bool Keyboard::operator[](size_t idx) const
{
	return this->keys[idx];
}

std::string Keyboard::toString() const
{
    std::string out;
    bool first = true;

    for (size_t i = 0; i < length; i++)
    {
        if (keys[i])
        {
            if (!first)
                out += ", ";

            // pokud je jméno definované, použij ho
            if (i < std::size(keyNames))
                out += keyNames[i];
            else
                out += "Unknown";

            first = false;
        }
    }

    return out;
}



