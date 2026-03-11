#pragma once

#include <cstdint>

class Color {
public:

	constexpr Color(uint32_t raw) : raw(raw) {}
	uint32_t raw;

	static constexpr Color Red() { return Color(0x00ff0000); }
	static constexpr Color Green() { return Color(0x0000ff00); }
	static constexpr Color Blue() { return Color(0x000000ff); }
	static constexpr Color White() { return Color(0x00ffffff); }
	static constexpr Color Black() { return Color(0x00000000); }
};