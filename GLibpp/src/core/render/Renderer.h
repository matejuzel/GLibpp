#pragma once

#include <atomic>
#include "window/WindowWin32.h"
#include "core/render/RenderContext.h"
#include "core/render/RenderDevice.h"

class Renderer {
private:

	std::atomic<bool> running{false};
	WindowWin32* window = nullptr;
	RenderContext context;
	RenderDevice device;

public:

	Renderer() = delete;

	Renderer(WindowWin32* window) : window(window) {}

	void init(uint32_t width, uint32_t height)
	{
		context.setViewport(0, 0, width, height);
		device.init(window, context.getViewport().width, context.getViewport().height, 32);
		context.setDevice(&device);
	}

	void runRenderLoop()
	{
		running.store(true, std::memory_order_relaxed);
		while (isRunning())
		{
			context.beginFrame();
			/* // toto postupne pridame
			while (queue.pop(cmd)) {
				execute(cmd);
			}
			*/
			context.paint(); // toto bude nahrazeno spsc queue uz s konkretnimi renderovacimi prikazy

			context.endFrame();
		}
	}

	void stop() {
		running.store(false, std::memory_order_relaxed);
	}

	inline bool isRunning() const noexcept {
		return running.load(std::memory_order_relaxed);
	}

};

