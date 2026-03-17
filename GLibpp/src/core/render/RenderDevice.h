#pragma once

#include <memory>

class RenderDevice {
private:
    
    std::unique_ptr<DIBFramebuffer> framebuffer;
    std::unique_ptr<DIBRenderer> dibRenderer;

public:

    void init(WindowWin32* window, uint32_t width, uint32_t height, uint32_t bit)
    {
        framebuffer = std::make_unique<DIBFramebuffer>(width, height, bit);
        framebuffer->init(window->getHwnd());

        dibRenderer = std::make_unique<DIBRenderer>(*framebuffer.get());
        dibRenderer->clear(0x00000000);

        framebuffer->present();
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