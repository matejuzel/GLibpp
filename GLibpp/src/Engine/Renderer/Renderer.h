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


#include "RenderResourceManager.h"

#include "SlotArray.h"

template <typename Device>
class Renderer {
private:

    using Target_h = Device::TargetHandle;
    using Context = Device::Context;
    
    std::unique_ptr<Device> device;

    RenderResourceManager<Device> resources;

    Target_h framebuffer_h;
    Target_h depthbuffer_h;

public:
    Renderer(WindowWin32& window)
        : device(std::make_unique<Device>(window))
        , framebuffer_h(resources.targets.add(RenderTargetDescriptor::Framebuffer(window.getClientWidth(), window.getClientHeight())))
        , depthbuffer_h(resources.targets.add(RenderTargetDescriptor::Framebuffer(window.getClientWidth(), window.getClientHeight()))) // @todo tady pak zmenit na ::Depthbuffer(...) az bude implementovano
    {}

    void renderFrame()
    {
        
        Context ctx;

        ctx.target = &resources.targets.get(framebuffer_h);

        uint32_t width = ctx.target->descriptor.width;
        uint32_t height = ctx.target->descriptor.height;
        float aspect = static_cast<float>(width) / static_cast<float>(height);

        ctx.view = Mtx4::LookAt(
            Vec4(5,5,5,1),
            Vec4(0,0,0,1),
            Vec4(0,1,0,0)
        );
        
        ctx.target->descriptor.width;

        ctx.projection = Mtx4::Perspective(
            45.0f,
            aspect,
            0.01f,
            100.0f
        );

        ctx.viewport = {
            0,0,
            width,height
        };

        ctx.clearColor = Color::Grayscale(0.2f);

        device->clear(ctx);
        device->draw(ctx);
        device->present(ctx);
    }

    void resize(uint32_t width, uint32_t height)
    {
        resources.targets.reset(framebuffer_h, RenderTargetDescriptor::Framebuffer(width, height));
    }
};
