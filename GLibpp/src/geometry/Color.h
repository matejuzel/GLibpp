#pragma once

#include <cstdint>

class Color {
public:
	Color(uint32_t raw) : raw(raw) {}
	uint32_t raw;
};