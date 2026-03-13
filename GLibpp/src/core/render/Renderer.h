#pragma once

#include <iostream>
#include "core/render/RenderCommand.h"
#include "core/render/RenderContext.h"
#include "utils/datastruct/SPSCQueue.h"
#include "core/Types.h"
#include "window/WindowWin32.h"
#include "geometry/Mesh.h"

class App; // dopredna deklarace kvuli cyklicke zavislosti s App.h

class Renderer {
public:

	Renderer(WindowWin32* window) : window(window), done(false) {}

	void runRenderLoop() {}

	/*
	RenderCommandBufferTB_t& getRenderCommandBufferRef() {
		return this->renderCommandBuffer;
	}

	RenderCommandQueueSPSC_t& getRenderCommandQueue() {
		return this->renderCommandQueue;
	}

	void drawScene();

	void registerMesh(Mesh* mesh, uint32_t meshId);
	*/

	

	void drawMesh(WindowWin32* window, const Mesh& mesh, const Mtx4& matrixMVP, const Material& material) const;

	void stop() {
		done.store(true);
	}

	bool isRunning() const {
		return !done.load();
	}



	WindowWin32* window;
	RenderContext renderContext;

	std::atomic<bool> done;

	/*
	RenderCommandBufferTB_t renderCommandBuffer;
	RenderCommandQueueSPSC_t renderCommandQueue;
	*/
};

