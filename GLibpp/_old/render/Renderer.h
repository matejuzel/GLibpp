#pragma once

#include <atomic>
#include <memory>
//#include "window/WindowWin32.h"
#include "render/IRenderTarget.h"
#include "render/backendDIB/RenderContextDIB.h"
#include "render/backendDIB/RenderDeviceDIB.h"
#include "render/backendDIB/RenderTargetDIB.h"
#include "utils/timer/FixedTimestep.h"
#include "utils/timer/Fps.h"
#include "utils/timer/HighResTimer.h"


class Renderer {
private:

	IRenderDevice* device = nullptr;

	//WindowWin32* window = nullptr;
	std::atomic<bool> running{ false };

public:

	Renderer() = delete;

	Renderer(IRenderDevice* device, uint32_t width, uint32_t height) : device(device) {

		//context.setDevice(device);
		//context.setViewport(0, 0, width, height);

	}

	void runRenderLoop()
	{
		running.store(true, std::memory_order_relaxed);

		Fps fps;
		FixedTimestep fpsTimestep(30.0);
		HighResTimer timer;

		while (isRunning())
		{

			//std::shared_ptr<IRenderTarget> rtMain = device->createRenderTarget(...);

			auto context = device->createRenderContext();
			context->beginFrame();

			/* // tady pak bude tripple buffered render command buffer
			while (queue.pop(cmd)) {
				execute(cmd);
			}
			*/

			{
				// docasne -> pak pouzijeme tripple buffered render commands ( EngineApp -> Renderer )
				device->draw2dCircle(
					(context->getFrameCount()) % context->getViewport().width,
					(context->getFrameCount() / 10 % context->getViewport().height),
					10,
					0x00550000
				);
			}

			context->endFrame();

			/*
			fps.tick();

			double frameTime = timer.tick();
			if (frameTime > 0.25) frameTime = 0.25;
			for (int i = 0; i < fpsTimestep.consume(frameTime); i++)
			{
				this->window->setTitle("Render FPS: " + std::to_string(fps.getFps()));
			}
			*/
		}
	}

	void stop() {
		running.store(false, std::memory_order_relaxed);
	}

	inline bool isRunning() const noexcept {
		return running.load(std::memory_order_relaxed);
	}

};

