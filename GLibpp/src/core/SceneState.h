#pragma once

#include "math/Mtx4.h"
#include "math/Vec4.h"
#include "geometry/Mesh.h"
#include "core/render/RenderCommand.h"

class SceneState {

public:
	
	SceneState() = default;

	uint32_t width = 1920;
	uint32_t height = 1080;
	Mtx4 projection;
	Mtx4 view;
	std::vector<Mesh> meshesStatic;
	std::vector<Mesh> meshesDynamic;
	Viewport viewport;
	float velocityMove;

};