#pragma once

#include "math/Mtx4.h"
#include "math/Vec4.h"
#include "geometry/Mesh.h"
#include "core/render/RenderCommand.h"
#include "utils/datastruct/Viewport.h"
#include "scene/Camera.h"
#include "scene/Light.h"
#include "geometry/MeshInstance.h"

class SceneState {

public:
	
	SceneState() = default;

	/*
	void addEntity(const MeshInstance& entity) {
		entities.push_back(entity);
	}
	*/

	void addLight(const Light& light) {
		lights.push_back(light);
	}

	const Camera& getCamera() const 
	{
		return camera;
	}
	Camera& getCamera()
	{
		return camera;
	}


private:

	Camera camera;
	//std::vector<MeshInstance> entities;
	std::vector<Light> lights;


};