#pragma once

#include <iostream>
#include "core/render/RenderCommand.h"
#include "core/render/RenderContext.h"
#include "utils/datastruct/SPSCQueue.h"
#include "core/Types.h"
#include "window/WindowBuilder.h"

class App; // dopredna deklarace kvuli cyklicke zavislosti s App.h

class Renderer {
public:

	Renderer(WindowBuilder* window) : window(window), done(false) {}

	void runRenderLoop();

	RenderCommandBufferTB_t& getRenderCommandBufferRef() {
		return this->renderCommandBuffer;
	}

	RenderCommandQueueSPSC_t& getRenderCommandQueue() {
		return this->renderCommandQueue;
	}

	void drawScene();

	void registerMesh(Mesh* mesh, uint32_t meshId);

	void drawMesh(uint32_t meshId, Mtx4 transformation);

	Mesh* getMesh(uint32_t meshId) const {
		if (meshId >= meshRegistry.size()) return nullptr;
		return meshRegistry[meshId];
	}

	void stop() {
		done.store(true);
	}

	bool isRunning() const {
		return !done.load();
	}



	WindowBuilder* window;

	RenderContext renderContext;
	std::vector<Mesh*> meshRegistry; // chtelo by to mit double bufferovane

	std::atomic<bool> done;
	RenderCommandBufferTB_t renderCommandBuffer;
	RenderCommandQueueSPSC_t renderCommandQueue;
};

