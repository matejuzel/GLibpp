#pragma once

#include "Camera.h"
#include "Mathematics.h"
#include "ResourceHandles.h"
#include <type_traits>

// handly demo renderables - trivialne kopirovatelne, zadne alokace pri publish/Slerp
// jedna instance se smi kreslit vicekrat (wheel je sdilena vsemi 4 koly)
struct SceneRenderables {
	MeshInstanceHandle gridWave  = MESH_INSTANCE_INVALID;
	MeshInstanceHandle carBody   = MESH_INSTANCE_INVALID;
	MeshInstanceHandle wheel     = MESH_INSTANCE_INVALID;
	MeshInstanceHandle icosphere = MESH_INSTANCE_INVALID;
	MeshInstanceHandle icrBeam   = MESH_INSTANCE_INVALID;
};

struct Scene {

	Camera camera;

	CarTransformation car;

	SceneRenderables renderables;


	Mtx4 modelMatrix;
	Mtx4 modelMatrix2;
	Mtx4 modelMatrix3;

	Mtx4 matrixVehicle = Mtx4::Identity();
	Mtx4 matrixWheel01 = Mtx4::Identity().translate(-1.0f, 0.5f,  2.0f).rotateZ(GLibpp::Math::deg2rad(90.0f));
	Mtx4 matrixWheel02 = Mtx4::Identity().translate( 1.0f, 0.5f,  2.0f).rotateZ(GLibpp::Math::deg2rad(90.0f));
	Mtx4 matrixWheel03 = Mtx4::Identity().translate(-1.0f, 0.5f, -2.0f).rotateZ(GLibpp::Math::deg2rad(90.0f));
	Mtx4 matrixWheel04 = Mtx4::Identity().translate( 1.0f, 0.5f, -2.0f).rotateZ(GLibpp::Math::deg2rad(90.0f));

	Mtx4 matrixSteer = Mtx4::Identity();
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
		sceneInterpolated.car = Slerp(a.car, b.car, t);

		return sceneInterpolated;
	}

};

// Scene se kopiruje pri kazdem publishi (60 Hz) i interpolaci (kazdy frame)
// - nesmi vlastnit zadna heapova data (meshe patri do ResourceManageru, Scene nese jen handly)
static_assert(std::is_trivially_copyable_v<Scene>);


struct LogicTickInfo {
	double lastLogicTick = 0.0;
};

struct LogicState {
	LogicTickInfo tickInfo;
	Scene scene;
};
