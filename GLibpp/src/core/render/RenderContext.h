#pragma once

//#include "RenderCommand.h"
#include "utils/datastruct/Viewport.h"
#include "window/DIB/DIBFramebuffer.h"
#include "window/DIB/DIBRenderer.h"
#include "window/WindowWin32.h"
#include "math/Mtx4.h"
#include <memory>

class RenderContext {

private:

    uint32_t frameCount = 0;

    Viewport viewport;
    Mtx4 projection;
    Mtx4 view;

public:

    // framebuffer a dibRenderer by mel pak vlastnit RenderDevice
    std::unique_ptr<DIBFramebuffer> framebuffer;
    std::unique_ptr<DIBRenderer> dibRenderer;

public:
    RenderContext() :
        viewport(0, 0, 800, 600),
        projection(Mtx4::identity()),
        view(Mtx4::identity())
    {}

    uint32_t getFrameCount() const noexcept { return frameCount; }
    const Viewport& getViewport() const noexcept { return viewport; }

    void init(WindowWin32* window)
    {
        framebuffer = std::make_unique<DIBFramebuffer>(viewport.width, viewport.height, 32);
        framebuffer->init(window->getHwnd());
        
        dibRenderer = std::make_unique<DIBRenderer>(*framebuffer.get());
        dibRenderer->clear(0x00000000);

        framebuffer->present();
    }

    void beginFrame()
    {
        clear();
    }

    void endFrame()
    {
        present();
        frameCount++;
    }

    void setViewport(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height) {
        viewport.offsetX = offsetX;
        viewport.offsetY = offsetY;
        viewport.width = width;
        viewport.height = height;
    }


    // presunout do RenderDevice
    void clear()
    {
        dibRenderer->clear(0x00000000);
    }

    // presunout do RenderDevice
    void draw2dCircle(uint32_t x, uint32_t y, uint32_t size, uint32_t color)
    {
        dibRenderer->drawCircle(x, y, size, color);
    }

    // presunout do RenderDevice
    void present()
    {
        framebuffer.get()->present();
    }


};


