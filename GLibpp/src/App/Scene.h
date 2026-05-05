#pragma once

#include "Camera.h"

struct Scene {

	Camera camera;
	Mtx4 modelMatrix;

	float rotationSpeed = 0.0f;
	float cameraSpeed = 0.0f;
	float cameraRotationSpeed = 0.0f;
	float test = 1.0f;

	friend Scene Slerp(const Scene& a, const Scene& b, float t) {
	
		Scene sceneInterpolated;
		sceneInterpolated.camera = Slerp(a.camera, b.camera, t);
		sceneInterpolated.test = a.test + (b.test - a.test) * t;
		sceneInterpolated.modelMatrix = Mtx4::Slerp(a.modelMatrix, b.modelMatrix, t);
		return sceneInterpolated;
	}

};
