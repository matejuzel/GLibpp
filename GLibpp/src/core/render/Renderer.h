#pragma once

#include <iostream>
#include "core/render/RenderCommand.h"

typedef SPSCQueue<RenderCommand::Command, 1024> RenderCommandQueueSPSC_t;
typedef TripleBuffer<RenderCommand::Buffer> RenderCommandBufferTB_t;

class Renderer {
public:

	Renderer():done(false) {}

	void runRenderLoop() {

		while (!done.load(std::memory_order_relaxed)) {

			// precteme a provedeme dynamicke prikazy z fronty
			while (true) {
				
				RenderCommand::Command command;
				if (!renderCommandQueue.pop(command)) break;
				RenderCommand::exec(command);
			}

			// precteme a provedeme staticke prikazy z bufferu
			const auto& commands = renderCommandBuffer.readBuffer();
			commands.execute();
			
			drawScene();
		}
	}

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

private:
	std::atomic<bool> done;

	RenderCommandBufferTB_t renderCommandBuffer;
	RenderCommandQueueSPSC_t renderCommandQueue;
};

