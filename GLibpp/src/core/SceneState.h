#pragma once

#include "math/Mtx4.h"
#include "math/Vec4.h"

class SceneState {

public:
	
	SceneState() {
	
		this->transformation.identity();
	};


	Mtx4 transformation;

	float velocityMove = 0.0f;
	//float velocityRotation = 0.0f;

};