#pragma once

#include "Camera.h"
#include "Car.h"
#include "ResourceHandles.h"
#include "ZeroAllocTripleBuffer.h"
#include <type_traits>

// handly demo renderables - trivialne kopirovatelne, zadne alokace pri publish/Slerp
// jedna instance se smi kreslit vicekrat (wheel je sdilena vsemi 4 koly)
struct SceneRenderables {
	GLibpp::Assets::MeshInstanceHandle gridWave  = GLibpp::Assets::MESH_INSTANCE_HANDLE_INVALID;
	GLibpp::Assets::MeshInstanceHandle carBody   = GLibpp::Assets::MESH_INSTANCE_HANDLE_INVALID;
	GLibpp::Assets::MeshInstanceHandle wheel     = GLibpp::Assets::MESH_INSTANCE_HANDLE_INVALID;
	GLibpp::Assets::MeshInstanceHandle icosphere = GLibpp::Assets::MESH_INSTANCE_HANDLE_INVALID;
	GLibpp::Assets::MeshInstanceHandle icrBeam   = GLibpp::Assets::MESH_INSTANCE_HANDLE_INVALID;
	GLibpp::Assets::MeshInstanceHandle test      = GLibpp::Assets::MESH_INSTANCE_HANDLE_INVALID;
	GLibpp::Assets::MeshInstanceHandle texPanel  = GLibpp::Assets::MESH_INSTANCE_HANDLE_INVALID;

	// textura panelu - binduje se commandem SetTexture pro texturovany pass
	GLibpp::Assets::TextureHandle panelTexture   = GLibpp::Assets::TEXTURE_HANDLE_INVALID;
};

struct Scene {

	GLibpp::Geometry::Camera camera;

	Car car;

	SceneRenderables renderables;

	friend Scene Slerp(const Scene& a, const Scene& b, float t) {

		// defaultne vse na current
		Scene sceneInterpolated = b;

		// volitelne jednotlive objekty interpolovat; jmeno rika, co se deje:
		// kamera se lerpuje (pozice + uhly), auto slerpuje heading quaternion
		sceneInterpolated.camera = Lerp(a.camera, b.camera, t);
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
using BufferedLogicState = GLibpp::Core::ZeroAllocTripleBuffer<LogicState>;
