#pragma once

#include <iostream>
#include "core/render/RenderCommand.h"
#include "utils/datastruct/SPSCQueue.h"
#include "core/Types.h"
#include "window/WindowBuilder.h"

class App; // dopredna deklarace kvuli cyklicke zavislosti s App.h

class Renderer {
public:

	Renderer():done(false) {}

	void runRenderLoop();

	RenderCommandBufferTB_t& getRenderCommandBufferRef() {
		return this->renderCommandBuffer;
	}

	RenderCommandQueueSPSC_t& getRenderCommandQueue() {
		return this->renderCommandQueue;
	}

	void drawScene() {
	
	}

	void stop() {
		done.store(true);
	}

	bool isRunning() const {
		return !done.load();
	}

private:
	std::atomic<bool> done;

	RenderCommandBufferTB_t renderCommandBuffer;
	RenderCommandQueueSPSC_t renderCommandQueue;
};

