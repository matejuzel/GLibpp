#pragma once
#include <string>

class Keyboard {

public:

	static constexpr size_t length = 256;
	static const char* keyNames[length];

private:

	bool keys[length] = {};

public:

	Keyboard();

	void keyDown(size_t code);
	void keyUp(size_t code);
	bool operator[](size_t idx) const;
	std::string toString() const;
};
