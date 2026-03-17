#pragma once

//#include "RenderCommand.h"
#include "utils/datastruct/Viewport.h"
#include "window/DIB/DIBFramebuffer.h"
#include "window/DIB/DIBRenderer.h"
#include "window/WindowWin32.h"
#include "core/render/RenderDevice.h"
#include "math/Mtx4.h"
#include <memory>

class RenderContext {

private:

    uint32_t frameCount = 0;

    Viewport viewport;
    Mtx4 projection;
    Mtx4 view;

    RenderDevice* device = nullptr;

public:
    RenderContext() :
        viewport(0, 0, 800, 600),
        projection(Mtx4::identity()),
        view(Mtx4::identity())
    {}

    void setDevice(RenderDevice* device) {
        this->device = device;
    }

    uint32_t getFrameCount() const noexcept { return frameCount; }
    const Viewport& getViewport() const noexcept { return viewport; }

    void beginFrame()
    {
        device->clear();
    }

    void endFrame()
    {
        device->present();
        frameCount++;
    }

    void setViewport(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height) {
        viewport.offsetX = offsetX;
        viewport.offsetY = offsetY;
        viewport.width = width;
        viewport.height = height;
    }

    void paint()
    {

        device->draw2dCircle(
            (getFrameCount()) % getViewport().width,
            (getFrameCount() / 10 % getViewport().height),
            10,
            0x00ff0000
        );
    }




};


