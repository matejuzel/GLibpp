#pragma once

#include <cstdint>

#include "IRenderTarget.h"
#include "IRenderDevice.h"
#include "IRenderContext.h"
#include "RenderTargetDIB.h"
#include "RenderContextDIB.h"
#include "RenderDeviceDIB.h"
#include "RenderCommandList.h"
#include "RenderResourceManager.h"

class Renderer {
private:
    IRenderDevice& device;
    RenderResourceManager resources;
    uint32_t width = 0;
    uint32_t height = 0;

public:
    Renderer(IRenderDevice& device, uint32_t width, uint32_t height)
        : device(device), width(width), height(height) {
    }

    void runLoop(const RenderCommandList& commandList) {
        auto backbuffer = device.acquireBackbuffer();

        auto ctx = device.beginContext();
        ctx->renderCommandList(commandList);
        ctx->publish();

        device.present(backbuffer);
    }
};