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
			.applyTransformation();

		// position the whole scene (or mesh instance) via scene transformation
		this->transformation = Mtx4::translation(0.0f, 20.0f, 0.0f);
	};

	Mesh mesh;
	Mtx4 transformation;

	// per-scene view/projection/viewport
	Mtx4 lookAt;
	Mtx4 projection;
	Mtx4 viewport;

	float velocityMove = 0.0f;
};