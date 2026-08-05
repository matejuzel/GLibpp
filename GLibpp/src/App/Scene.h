#pragma once

#include "Camera.h"
#include "CarTransformation.h"
#include "ResourceHandles.h"
#include "ZeroAllocTripleBuffer.h"
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

	friend Scene Slerp(const Scene& a, const Scene& b, float t) {

		// defaultne vse na current
		Scene sceneInterpolated = b;

		// volitelne jednotlive objekty interpolovat
		sceneInterpolated.camera = Slerp(a.camera, b.camera, t);
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

// SPSC most mezi logickym a render vlaknem (jediny sdileny stav App <-> Renderer)
using LogicStateBuffered = ZeroAllocTripleBuffer<LogicState>;
