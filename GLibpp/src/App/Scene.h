#pragma once

#include "Camera.h"

struct Scene {

	Camera camera;
	Mtx4 modelMatrix;
	Mtx4 modelMatrix2;

	float rotationSpeed = 0.0f;
	float cameraSpeed = 0.0f;
	float cameraRotationSpeed = 0.0f;
	float test = 1.0f;

	friend Scene Slerp(const Scene& a, const Scene& b, float t) {
	
		Scene sceneInterpolated;
		sceneInterpolated.camera = Slerp(a.camera, b.camera, t);
		sceneInterpolated.test = a.test + (b.test - a.test) * t;
		sceneInterpolated.modelMatrix = Mtx4::Slerp(a.modelMatrix, b.modelMatrix, t);
		sceneInterpolated.modelMatrix2 = Mtx4::Slerp(a.modelMatrix2, b.modelMatrix2, t);
		return sceneInterpolated;
	}

};


struct LogicTickInfo {
	double lastLogicTick;
};

struct LogicState {
	LogicTickInfo tickInfo;
	Scene scene;
};
