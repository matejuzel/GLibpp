#pragma once

#include "Camera.h"
#include "Mathematics.h"

struct Scene {

	Camera camera;
	Mtx4 modelMatrix;
	Mtx4 modelMatrix2;
	Mtx4 modelMatrix3;

	Mtx4 matrixVehicle = Mtx4::Identity();
	Mtx4 matrixWheel01 = Mtx4::Identity().translate(-1.0f, 0.5f,  2.0f).rotateZ(GLibpp::Math::deg2rad(90.0f));
	Mtx4 matrixWheel02 = Mtx4::Identity().translate( 1.0f, 0.5f,  2.0f).rotateZ(GLibpp::Math::deg2rad(90.0f));
	Mtx4 matrixWheel03 = Mtx4::Identity().translate(-1.0f, 0.5f, -2.0f).rotateZ(GLibpp::Math::deg2rad(90.0f));
	Mtx4 matrixWheel04 = Mtx4::Identity().translate( 1.0f, 0.5f, -2.0f).rotateZ(GLibpp::Math::deg2rad(90.0f));
	float speed = 0.0f;




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
