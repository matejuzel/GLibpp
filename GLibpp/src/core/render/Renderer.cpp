
#include "core/App.h" 
#include "core/render/Renderer.h"
#include "utils/timer/Fps.h"

/*
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
*/

void Renderer::drawMesh(WindowBuilder* window, const Mesh& mesh, const Mtx4& matrixMVP, const Material& material) const
{
	if (window == nullptr) return;

	float x = (float)renderContext.viewport.offsetX;
	float y = (float)renderContext.viewport.offsetY;
	float w2 = (float)renderContext.viewport.width * 0.5f;
	float h2 = (float)renderContext.viewport.height * 0.5f;

	const auto& verts = mesh.getVertexBuffer();
	const auto& indices = mesh.getIndexBuffer();

	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		const Vec4& aPos = verts[indices[i + 0]];
		const Vec4& bPos = verts[indices[i + 1]];
		const Vec4& cPos = verts[indices[i + 2]];

		// MVP transformace
		Vec4 a_ = matrixMVP * aPos;
		Vec4 b_ = matrixMVP * bPos;
		Vec4 c_ = matrixMVP * cPos;

		// Trivial frustum reject
		{
			if (a_.w <= 0 || b_.w <= 0 || c_.w <= 0)
				continue;

			bool insideA =
				a_.x >= -a_.w && a_.x <= a_.w &&
				a_.y >= -a_.w && a_.y <= a_.w &&
				a_.z >= -a_.w && a_.z <= a_.w;

			bool insideB =
				b_.x >= -b_.w && b_.x <= b_.w &&
				b_.y >= -b_.w && b_.y <= b_.w &&
				b_.z >= -b_.w && b_.z <= b_.w;

			bool insideC =
				c_.x >= -c_.w && c_.x <= c_.w &&
				c_.y >= -c_.w && c_.y <= c_.w &&
				c_.z >= -c_.w && c_.z <= c_.w;

			if (!insideA || !insideB || !insideC)
				continue;
		}

		// Divide by W (NDC)
		a_.divideW();
		b_.divideW();
		c_.divideW();

		// Viewport transform
		a_.x = a_.x * w2 + (w2 + x);
		a_.y = a_.y * -h2 + (h2 + y);

		b_.x = b_.x * w2 + (w2 + x);
		b_.y = b_.y * -h2 + (h2 + y);

		c_.x = c_.x * w2 + (w2 + x);
		c_.y = c_.y * -h2 + (h2 + y);

		// Draw
		window->DIB_drawTriangle(a_, b_, c_, 0xffff0000, true);
	}
}

/* //puvodni verze
void Renderer::drawMesh(uint32_t meshId, Mtx4 transformation) {
		
	if (window == nullptr) return;

	auto& mesh = *meshRegistry[meshId];

	float x = (float) renderContext.viewport.offsetX;
	float y = (float) renderContext.viewport.offsetY;
	float w2 = (float) renderContext.viewport.width / 2.0f;
	float h2 = (float) renderContext.viewport.height / 2.0f;

	Mtx4 matrixMVP = renderContext.projection * renderContext.modelview * transformation;

	for (const Triangle& tri : mesh.tris) {

		// MVP transformation (Object space -> World space -> View space -> Clip-space)
		Vec4 a_ = matrixMVP * tri.a.pos;
		Vec4 b_ = matrixMVP * tri.b.pos;
		Vec4 c_ = matrixMVP * tri.c.pos;
		
		// Trivial frustum reject in clip-space.
		{
			
			// Zahodime trojuhelnik, pokud libovolny vrchol lezi mimo clip volume.
			// POZN: Toto neni skutecny clipping - trojuhelnik se pouze zahodi.
			// Spravne reseni by bylo near-plane clipping (Sutherland-Hodgman).

			if (a_.w <= 0 || b_.w <= 0 || c_.w <= 0) continue;

			bool insideA =
				a_.x >= -a_.w && a_.x <= a_.w &&
				a_.y >= -a_.w && a_.y <= a_.w &&
				a_.z >= -a_.w && a_.z <= a_.w;

			bool insideB =
				b_.x >= -b_.w && b_.x <= b_.w &&
				b_.y >= -b_.w && b_.y <= b_.w &&
				b_.z >= -b_.w && b_.z <= b_.w;

			bool insideC =
				c_.x >= -c_.w && c_.x <= c_.w &&
				c_.y >= -c_.w && c_.y <= c_.w &&
				c_.z >= -c_.w && c_.z <= c_.w;

			if (!insideA || !insideB || !insideC) continue;
		}

		// divide by W (Clip-space -> NDC)
		a_.divideW();
		b_.divideW();
		c_.divideW();

		// Viewport transform (NDC -> Screen)
		{
			// toto je rozepsane nasobeni touto viewport matici
			// zde ma vyznam z hlediska rychlosti takhle rozepsat
			// Mtx4 viewportMatrix = Mtx4{
			//     w2, 0.0f, 0.0f, w2 + x,
			//     0.0f,  -h2, 0.0f, h2 + y,
			//     0.0f, 0.0f, 1.0f,   0.0f,
			//     0.0f, 0.0f, 0.0f,   1.0f
			// };

			a_.x = a_.x * w2 + (w2 + x);
			a_.y = a_.y * -h2 + (h2 + y);

			b_.x = b_.x * w2 + (w2 + x);
			b_.y = b_.y * -h2 + (h2 + y);

			c_.x = c_.x * w2 + (w2 + x);
			c_.y = c_.y * -h2 + (h2 + y);
		}

		// render triangle
		window->DIB_drawTriangle(a_, b_, c_, 0xffff0000, true);
	}
	
}
*/
