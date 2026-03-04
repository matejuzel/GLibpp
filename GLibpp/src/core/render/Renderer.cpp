
#include "core/App.h" 
#include "core/render/Renderer.h"

void Renderer::runRenderLoop() {

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

		App::instance().getWindowPtr()->DIB_drawBitmap();
		//this->win->DIB_drawBitmap();

		drawScene();
	}
}
