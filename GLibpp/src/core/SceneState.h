#pragma once

#include "math/Mtx4.h"
#include "math/Vec4.h"
#include "geometry/Mesh.h"

class SceneState {

public:
	
	SceneState() {
		
		this->transformation.identity();

		// apply only scaling to mesh vertices (keep translation as scene transformation)
		mesh.addCube(1)
			.transform(Mtx4::scaling(1.4f, 1.1f, 1.2f))
			.applyTransformation()
			;

		mesh2.addCube(0.2f);

		flat.addNet(10)
			.transform(Mtx4::translation(0, -1, 0))
			.transform(Mtx4::scaling(10,10,10))
			.applyTransformation()
			;

	};

	Mesh mesh, mesh2;
	Mesh flat;

	Mtx4 transformation;

	Mtx4 viewPersp;
	Mtx4 projectionPersp;
	Mtx4 viewportPersp;

	Mtx4 viewTop;
	Mtx4 projectionTop;
	Mtx4 viewportTop;

	float velocityMove = 0.0f;
};