#pragma once
#include <cstddef>
#include <Windows.h>

// Interní engine klávesy
enum KeyMap : size_t
{
    KEY_UNKNOWN = 0,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_ESC,
    KEY_SPACE,
    KEY_ENTER,
    KEY_SHIFT,
    KEY_CTRL,
    // případně další KEY_...
};

// Mapovací tabulka: VK_xxx → KeyMap
static constexpr KeyMap KEYMAP[256] =
{
    /* 0x00 */ KEY_UNKNOWN,
    /* 0x01 */ KEY_UNKNOWN,
    /* 0x02 */ KEY_UNKNOWN,
    /* 0x03 */ KEY_UNKNOWN,
    /* 0x04 */ KEY_UNKNOWN,
    /* 0x05 */ KEY_UNKNOWN,
    /* 0x06 */ KEY_UNKNOWN,
    /* 0x07 */ KEY_UNKNOWN,
    /* 0x08 */ KEY_UNKNOWN,
    /* 0x09 */ KEY_UNKNOWN,
    /* 0x0A */ KEY_UNKNOWN,
    /* 0x0B */ KEY_UNKNOWN,
    /* 0x0C */ KEY_UNKNOWN,
    /* 0x0D */ KEY_ENTER,     // VK_RETURN
    /* 0x0E */ KEY_UNKNOWN,
    /* 0x0F */ KEY_UNKNOWN,
    /* 0x10 */ KEY_SHIFT,     // VK_SHIFT
    /* 0x11 */ KEY_CTRL,      // VK_CONTROL
    /* 0x12 */ KEY_UNKNOWN,
    /* 0x13 */ KEY_UNKNOWN,
    /* 0x14 */ KEY_UNKNOWN,
    /* 0x15 */ KEY_UNKNOWN,
    /* 0x16 */ KEY_UNKNOWN,
    /* 0x17 */ KEY_UNKNOWN,
    /* 0x18 */ KEY_UNKNOWN,
    /* 0x19 */ KEY_UNKNOWN,
    /* 0x1A */ KEY_UNKNOWN,
    /* 0x1B */ KEY_ESC,       // VK_ESCAPE
    /* 0x1C */ KEY_UNKNOWN,
    /* 0x1D */ KEY_UNKNOWN,
    /* 0x1E */ KEY_UNKNOWN,
    /* 0x1F */ KEY_UNKNOWN,
    /* 0x20 */ KEY_SPACE,     // VK_SPACE
    /* 0x21 */ KEY_UNKNOWN,
    /* 0x22 */ KEY_UNKNOWN,
    /* 0x23 */ KEY_UNKNOWN,
    /* 0x24 */ KEY_UNKNOWN,
    /* 0x25 */ KEY_LEFT,      // VK_LEFT
    /* 0x26 */ KEY_UP,        // VK_UP
    /* 0x27 */ KEY_RIGHT,     // VK_RIGHT
    /* 0x28 */ KEY_DOWN,      // VK_DOWN
    /* 0x29 */ KEY_UNKNOWN,
    /* 0x2A */ KEY_UNKNOWN,
    /* 0x2B */ KEY_UNKNOWN,
    /* 0x2C */ KEY_UNKNOWN,
    /* 0x2D */ KEY_UNKNOWN,
    /* 0x2E */ KEY_UNKNOWN,
    /* 0x2F */ KEY_UNKNOWN,

    // Zbytek 0x30–0xFF nastavíme na KEY_UNKNOWN
};

static_assert(sizeof(KEYMAP) / sizeof(KEYMAP[0]) == 256, "KEYMAP must have 256 entries");
