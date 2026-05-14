#pragma once

#include "Camera.h"

struct Scene {

	Camera camera;
	Mtx4 modelMatrix;
	Mtx4 modelMatrix2;
	Mtx4 modelMatrix3;

	float rotationSpeed = 0.0f;
	float cameraSpeed = 0.0f;
	float cameraRotationSpeed = 0.0f;
	float test = 1.0f;

	friend Scene Slerp(const Scene& a, const Scene& b, float t) {
	
		// defaultne vse na current
		Scene sceneInterpolated = b;

		// volitelne jednotlive objekty interpolovat
		sceneInterpolated.camera = Slerp(a.camera, b.camera, t);
		sceneInterpolated.test = a.test + (b.test - a.test) * t;

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
