#pragma once

#include <memory>
#include "geometry/Color.h"

class Material {
public:

	static std::unique_ptr<Material> Create(Color color) {
		auto mat = std::make_unique<Material>();
		mat->color = color;
		return mat;
	}

	Material() : color(0x00ff0000) {}

	Color color;
	// Pozdìji...:
	// - shader handle
	// - textury
	// - uniformy
	// - barvy
	// - parametry
	// - pipeline state
};
