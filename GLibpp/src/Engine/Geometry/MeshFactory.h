#pragma once

#include "Mesh.h"

namespace GLibpp::Geometry {

class MeshFactory {

public:

	static Mesh CreateQuad(float size = 1.0f);

	// ram kolem quadu: prstenec ze 4 tenkych pasu v XY rovine (8 vrcholu,
	// 8 trojuhelniku); vnitrni hrana = size - 2*thickness, vnejsi = size
	static Mesh CreateQuadFrame(float size = 1.0f, float thickness = 0.05f);

	static Mesh CreateTriangle(float size = 1.0f);

	static Mesh CreateCube(float side = 1.0f);
	static Mesh CreateSphere(float radius = 1.0f, uint32_t segments = 16);
	static Mesh CreateCylinder(float radius = 1.0f, float height = 1.0f, uint32_t segments = 16);
	static Mesh CreateIcosan(float radius = 1.0f);
	static Mesh CreateIcosphere(float radius = 1.0f, uint32_t subdivisions = 1);
	static Mesh CreateGrid(uint32_t size = 1, float distort = 0.0f);
	static Mesh CreateGridWave(uint32_t size = 1, float waveHeight = 0.5f, float time = 0.0f, float speed = 1.0f);

	// aktualizuje in-place vysku vlny meshe vytvoreneho pres CreateGridWave (stejne size!) - zadne alokace
	static void UpdateGridWave(Mesh& msh, uint32_t size, float waveHeight, float time, float speed);

};

}