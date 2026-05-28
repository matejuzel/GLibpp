#pragma once

#include "Mesh.h"

class MeshFactory {

public:

	static Mesh CreateQuad(float size = 1.0f);
	static Mesh CreateTriangle(float size = 1.0f);

	static Mesh CreateCube(float side = 1.0f);
	static Mesh CreateSphere(float radius = 1.0f, uint32_t segments = 16);
	static Mesh CreateCylinder(float radius = 1.0f, float height = 1.0f, uint32_t segments = 16);
	static Mesh CreateIcosan(float radius = 1.0f);
	static Mesh CreateIcosphere(float radius = 1.0f, uint32_t subdivisions = 1);
	static Mesh CreateGrid(uint32_t size = 1, float distort = 0.0f);
	static Mesh CreateGridWave(uint32_t size = 1, float waveHeight = 0.5f, float time = 0.0f, float speed = 1.0f);
	
};