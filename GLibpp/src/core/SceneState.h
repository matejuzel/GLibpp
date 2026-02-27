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
			.transform(Mtx4::scaling(1.4, 1.1, 1.2))
			//.applyTransformation()
			;

		mesh.addNet(10)
			.transform(Mtx4::scaling(4,4,4))
			//.applyTransformation()
			.transform(Mtx4::translation(0,-1,0))
			//.applyTransformation()
			;

	};

	Mesh mesh;
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