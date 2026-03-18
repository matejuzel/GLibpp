#pragma once

#include <atomic>
#include <memory>
#include "window/WindowWin32.h"
#include "core/render/RenderContext.h"
#include "core/render/IRenderDevice.h"
#include "core/render/RenderDeviceDIB.h"

class Renderer {
private:

	std::unique_ptr<IRenderDevice> device;
	RenderContext context;

	WindowWin32* window = nullptr;
	std::atomic<bool> running{ false };

public:

	Renderer() = delete;

	Renderer(WindowWin32* window, uint32_t width, uint32_t height) : window(window) {
		context.setViewport(0, 0, width, height);
		device = std::make_unique<RenderDeviceDIB>(window, width, height, 32);
		context.setDevice(device.get());
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

