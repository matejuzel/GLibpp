#pragma once

#include "RenderCommand.h"

class RenderContext {
public:

	RenderContext(WindowBuilder* window) : 
		window(window),
		viewport{ 0, 0, 800, 600 },
		projection{ Mtx4::identity() },
		modelview{ Mtx4::identity() }
	{}

	void registerMesh(Mesh* mesh, uint32_t meshId) {
		meshes[meshId] = mesh;
	}

	Mesh* getMesh(uint32_t meshId) const {
		
		auto it = meshes.find(meshId);
		if (it == meshes.end()) return nullptr;
		return it->second;
	}

	void drawMesh(uint32_t meshId) {
		
		Mesh* mesh = getMesh(meshId);

		if (mesh != nullptr) 
		{
			Mtx4 mvp = projection * modelview * mesh->transformation;
			//window.drawMesh(window, viewport, mvp, mesh); // @todo
		}
	}

	WindowBuilder* window;

private:

	
	std::unordered_map<uint32_t, Mesh*> meshes;
	Viewport viewport;
	Mtx4 projection;
	Mtx4 modelview;
};