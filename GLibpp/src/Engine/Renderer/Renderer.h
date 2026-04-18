#pragma once

#include "RenderTargetDescriptor.h"
#include "Viewport.h"
#include "Color.h"
#include "WindowWin32.h"

#include <cstdint>
#include <memory>
#include "Vec4.h"
#include "Mtx4.h"

// Backend Common
#include "RenderContext.h"
#include "RenderDeviceBase.h"
#include "RenderTargetBase.h"

// Backend DIB
#include "RenderDeviceDIB.h"

// Backend Stencil
#include "RenderDeviceStencil.h"

template <typename Device>
class Renderer {
private:

    using Context = RenderContext<Device, typename Device::Target>;

    std::unique_ptr<Device> device;
    std::unique_ptr<typename Device::Target> target;
    WindowWin32& window;
    Context ctx{};

public:
    Renderer(WindowWin32& window)
        : device(std::make_unique<Device>(window))
        , window(window)
    {
        target = device->createTarget(RenderTargetDescriptor::Framebuffer(
            window.getClientWidth(),
            window.getClientHeight()
        ));
   
    }

    void renderFrame()
    {
        
        ctx.target = target.get();

        ctx.view = Mtx4::LookAt(
            Vec4(5,5,5,1),
            Vec4(0,0,0,1),
            Vec4(0,1,0,0)
        );

        ctx.projection = Mtx4::Perspective(
            45.0f,
            (float)window.getClientWidth() / (float)window.getClientHeight(),
            0.01f,
            100.0f
        );

        ctx.viewport = {
            0,0,
            window.getClientWidth(),
            window.getClientHeight()
        };

        ctx.clearColor = Color::Grayscale(0.2f);


        device->clear(ctx, *target);
        device->draw(ctx, *target);
        device->present(ctx, *target);
    }

    void resize(uint32_t width, uint32_t height)
    {
        target = device->createTarget(RenderTargetDescriptor::Framebuffer(width, height));
    }
};
