#pragma once

#include "math/Mtx4.h"
#include "math/Vec4.h"
#include "geometry/Mesh.h"

class SceneState {

public:
	
	SceneState() {
		
		this->transformation.identity();
		mesh.addCube(1).transform(Mtx4::scaling(1.4, 1.1, 1.2)).applyTransformation();
	};

	Mesh mesh;
	Mtx4 transformation;

	float velocityMove = 0.0f;
};