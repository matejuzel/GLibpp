#pragma once

#include "math/Mtx4.h"
#include "math/Vec4.h"
#include "geometry/Mesh.h"
#include "core/render/RenderCommand.h"

class SceneState {

public:
	
	SceneState() {

		projection = Mtx4::perspective(
			45.0f * 3.14159f / 180.0f, 
			width / (float)height, 
			0.01f, 
			1000.0f
		);

		view = Mtx4::lookAt(
			Vec4(10.0f,  1.0f,  3.0f,  1.0f), 
			Vec4( 0.0f,  0.0f,  0.0f,  1.0f), 
			Vec4( 0.0f,  1.0f,  0.0f,  0.0f)
		);

		viewport = { 0, 0, width, height };

		meshesStatic = {
			Mesh().addNet(20).transform(Mtx4::scaling(5.0f, 5.0f, 5.0f)).applyTransformation(),
			Mesh().addCube(1.0f).transform(Mtx4::rotationY(0.9f)).applyTransformation(),
			Mesh().addCube(0.3f).transform(Mtx4::translation(2.0f, 1.0f, 0.6f)).applyTransformation(),
		};

		meshesDynamic = {
			Mesh().addCube(0.5f),
		};

		velocityMove = 0.0f;

	};

	uint32_t width = 1366;
	uint32_t height = 800;
	Mtx4 projection;
	Mtx4 view;
	std::vector<Mesh> meshesStatic;
	std::vector<Mesh> meshesDynamic;
	Viewport viewport;
	float velocityMove;

};