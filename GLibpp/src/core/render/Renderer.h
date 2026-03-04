#pragma once

#include "core/render/RenderCommand.h"

class Renderer {
public:

	Renderer() : done(false) {}

	void runRenderLoop() {
		while (!done.load()) {
			const auto& commands = renderCommandBuffer.readBuffer();
			commands.execute();
		}
	}

	TripleBuffer<RenderCommand::Buffer>& getRenderCommandBufferRef() {
		return this->renderCommandBuffer;
	}

	void stop() {
		done.store(true);
	}

private:
	std::atomic<bool> done;
	TripleBuffer<RenderCommand::Buffer> renderCommandBuffer;
};

