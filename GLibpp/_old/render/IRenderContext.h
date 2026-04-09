#pragma once

//#include "RenderCommand.h"
#include "utils/datastruct/Viewport.h"
#include "window/DIB/DIBFramebuffer.h"
#include "window/DIB/DIBRenderer.h"
#include "window/WindowWin32.h"
#include "render/IRenderDevice.h"
#include "math/Mtx4.h"
#include <memory>

class IRenderContext {

public:

    IRenderContext() = default;
	~IRenderContext() = default;

    virtual void setDevice(IRenderDevice* device) = 0;
    virtual uint32_t getFrameCount() const noexcept = 0;
    virtual const Viewport& getViewport() const noexcept = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void setViewport(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height) = 0;
};
