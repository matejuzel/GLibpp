#pragma once

#include "Vertex.h"

class Triangle {

public:

	Vertex a, b, c;

	Triangle(Vertex a, Vertex b, Vertex c) : a(a), b(b), c(c) {}

};