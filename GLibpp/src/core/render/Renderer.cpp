
#include "core/App.h" 
#include "core/render/Renderer.h"
#include "utils/timer/Fps.h"

void Renderer::runRenderLoop() {

	while (!done.load(std::memory_order_relaxed)) {

		// precteme a provedeme dynamicke prikazy z fronty
		while (true) {

			RenderCommand::Command command;
			if (!renderCommandQueue.pop(command)) break;
			RenderCommand::exec(command, *this);
		}

		// precteme a provedeme staticke prikazy z bufferu
		const auto& commands = renderCommandBuffer.readBuffer();
		commands.execute(*this);

		window->DIB_drawBitmap();
		App::instance().getFps().tick();
	}
}

void Renderer::drawScene() {

	// pro jednovlaknovou implementaci (Game loop a rendering v jednom vlakne)
	
	// precteme a provedeme dynamicke prikazy z fronty
	while (true) {

		RenderCommand::Command command;
		if (!renderCommandQueue.pop(command)) break;
		RenderCommand::exec(command, *this);
	}

	// precteme a provedeme staticke prikazy z bufferu
	const auto& commands = renderCommandBuffer.readBuffer();
	commands.execute(*this);

	window->DIB_drawBitmap();
	App::instance().getFps().tick();
}

void Renderer::registerMesh(Mesh* mesh, uint32_t meshId) {
	if (meshId >= meshRegistry.size()) {
		meshRegistry.resize(meshId + 1, nullptr);
	}
	meshRegistry[meshId] = mesh;
}


void Renderer::drawMesh(uint32_t meshId, Mtx4 transformation) {
		
	auto& mesh = *meshRegistry[meshId];

	float w2 = 400;
	float h2 = 300;

	Mtx4 viewportMatrix = Mtx4{
		  w2, 0.0f, 0.0f, w2 + 400,
		0.0f,   h2, 0.0f, h2 + 300,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	Mtx4 matrixMVP = renderContext.projection * renderContext.modelview * transformation;

	for (const Triangle& tri : mesh.tris) {

		Vec4 a_ = matrixMVP * tri.a.pos;
		Vec4 b_ = matrixMVP * tri.b.pos;
		Vec4 c_ = matrixMVP * tri.c.pos;

		a_.divideW();
		b_.divideW();
		c_.divideW();

		a_ = viewportMatrix * a_;
		b_ = viewportMatrix * b_;
		c_ = viewportMatrix * c_;

		if (window != nullptr) {
			window->DIB_drawTriangle(a_, b_, c_, 0xffff0000, true);
		}
		else {
			std::cout << "RenderCommand::drawMesh windowBuilder is null" << std::endl;

		}

	}
	
}

