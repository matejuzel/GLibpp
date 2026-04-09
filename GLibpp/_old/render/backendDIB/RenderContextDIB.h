#pragma once

#include "IRenderContext.h"

class RenderContextDIB : public IRenderContext
{
private:
    uint32_t frameCount = 0;

    Viewport viewport;
    Mtx4 projection;
    Mtx4 view;

    IRenderDevice* device = nullptr;

public:
    RenderContextDIB() :
        viewport(0, 0, 0, 0),
        projection(Mtx4::identity()),
        view(Mtx4::identity())
    {
    }

    void setDevice(IRenderDevice* device) {
        this->device = device;
    }

    uint32_t getFrameCount() const noexcept { return frameCount; }
    const Viewport& getViewport() const noexcept { return viewport; }

    void beginFrame()
    {
        device->clear(0x00000000);
    }

    void endFrame()
    {
        device->present();
        frameCount++;
    }

    void setViewport(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height)
    {
        viewport.offsetX = offsetX;
        viewport.offsetY = offsetY;
        viewport.width = width;
        viewport.height = height;

        device->setViewport(offsetX, offsetY, width, height);
    }
};