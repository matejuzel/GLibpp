#pragma once


enum class KeyCode {

	Unknown = 0,
	Left,
	Right,
	Up,
	Down,

	Esc,
	Space,

	Enter,
	Shift,
	Ctrl,
};


const char* toString(KeyCode key);