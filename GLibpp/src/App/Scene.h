#pragma once

#include "Camera.h"

struct Scene {

	Camera camera;
	Mtx4 modelMatrix;

	float rotationSpeed = 0.0f;


	float cameraSpeed = 0.0f;
	float cameraRotationSpeed = 0.0f;

};
