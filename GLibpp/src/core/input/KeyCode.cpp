

#pragma once

#include <unordered_map>
#include "core/input/KeyCode.h"


static std::unordered_map<KeyCode, const char*> names = {
	{ KeyCode::Left, "Left" },
	{ KeyCode::Right, "Right" },
	{ KeyCode::Up, "Up" },
	{ KeyCode::Down, "Down" },
	{ KeyCode::Esc, "Esc" },
	{ KeyCode::Space, "Space" },
	{ KeyCode::Enter, "Enter" },
	{ KeyCode::Shift, "Shift" },
	{ KeyCode::Ctrl, "Ctrl" }
};

const char* toString(KeyCode key)
{
	auto it = names.find(key);
	return it != names.end() ? it->second : "Unknown";
}


