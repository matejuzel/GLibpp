#pragma once

#include "Mesh.h"

class MeshFactory {

public:
	static Mesh CreateCube(float side);
	static Mesh CreateSphere(float radius);
	static Mesh CreateCylinder(float radius, float height, uint32_t segments);
	static Mesh CreateIcosan(float radius);
	static Mesh CreateIcosphere(float radius, uint32_t subdivisions);
	static Mesh CreateNet(uint32_t size, float distort = 0.0f);
	static Mesh CreateNetWave(uint32_t size, float waveHeight = 0.5f, float time = 0.0f, float speed = 1.0f);
	static Mesh CreateQuad(float size);
};