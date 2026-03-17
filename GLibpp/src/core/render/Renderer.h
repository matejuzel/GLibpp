#pragma once

#include <atomic>
#include "window/WindowWin32.h"
#include "core/render/RenderContext.h"

class Renderer {
private:

	std::atomic<bool> running{false};
	WindowWin32* window = nullptr;
	RenderContext context;

public:

	Renderer() = delete;

	Renderer(WindowWin32* window) : window(window) {}

	void resize(uint32_t width, uint32_t height) 
	{
		context.setViewport(0, 0, width, height);
	}

	void runRenderLoop() 
	{	
		context.init(window);

		running.store(true, std::memory_order_relaxed);
		while (isRunning()) 
		{
			context.beginFrame();
			/* // toto postupne pridame
			while (queue.pop(cmd)) {
				execute(cmd);
			}
			*/
			paint(); // toto bude nahrazeno spsc queue uz s konkretnimi renderovacimi prikazy

			context.endFrame();
		}
	}

	void paint() 
	{
		context.draw2dCircle(
			(context.getFrameCount()) % context.getViewport().width, 
			(context.getFrameCount()/10 % context.getViewport().height),
			10, 
			0x00ff0000
		);
	}

	void stop() {
		running.store(false, std::memory_order_relaxed);
	}

	inline bool isRunning() const noexcept {
		return running.load(std::memory_order_relaxed);
	}

};

