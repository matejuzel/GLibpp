
#include "core/App.h" 
#include "core/render/Renderer.h"
#include "utils/timer/Fps.h"

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

		App::instance().renderCtx->window->DIB_drawBitmap();
		App::instance().getFps().tick();
	}
}

inline void Renderer::drawScene() {

	// pro jednovlaknovou implementaci (Game loop a rendering v jednom vlakne)
	
	// precteme a provedeme dynamicke prikazy z fronty
	while (true) {

		RenderCommand::Command command;
		if (!renderCommandQueue.pop(command)) break;
		RenderCommand::exec(command);
	}

	// precteme a provedeme staticke prikazy z bufferu
	const auto& commands = renderCommandBuffer.readBuffer();
	commands.execute();

	App::instance().renderCtx->window->DIB_drawBitmap();
	App::instance().getFps().tick();
}
