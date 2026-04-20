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

    using Context = Device::Context;
    using Target_h = RenderResourceManager<Device>::Target_h;

    std::unique_ptr<Device> device;

    RenderResourceManager<Device> resources;

    Target_h framebuffer_h;
    Target_h depthbuffer_h;

    uint32_t frameCounter = 0;

public:
    Renderer(WindowWin32& window)
        : device(std::make_unique<Device>(window))
        , framebuffer_h(
            resources.targets.add(
                RenderTargetDescriptor::FramebufferRGBA32bit(window.getClientWidth(), window.getClientHeight())
            )
        )
        , depthbuffer_h(
                resources.targets.add(
                    RenderTargetDescriptor::Depthbuffer24bit(window.getClientWidth(), window.getClientHeight()
                )
            )
        )
    {
    }

    void renderFrame()
    {
        uint32_t width = framebuffer.descriptor.width;
        uint32_t height = framebuffer.descriptor.height;
        float aspect = static_cast<float>(width) / static_cast<float>(height);

        auto& framebuffer = resources.targets.get(framebuffer_h);

        auto ctx = device->createContext();

        ctx.clearColor = Color::Grayscale(0.4f);

        ctx.view = Mtx4::LookAt(
            Vec4(5+frameCounter/100.0f,5.0f,5.0f,1.0f),
            Vec4(0,0,0,1),
            Vec4(0,1,0,0)
        );

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

        device->clear(ctx, framebuffer);
        device->draw(ctx, framebuffer);
        device->present(ctx, framebuffer);

        frameCounter++;
    }

    void resize(uint32_t width, uint32_t height)
    {
        resources.targets.reset(framebuffer_h, RenderTargetDescriptor::FramebufferRGBA32bit(width, height));
        resources.targets.reset(depthbuffer_h, RenderTargetDescriptor::Depthbuffer24bit(width, height));
    }
};
