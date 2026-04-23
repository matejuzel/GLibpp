#pragma once

#include "RenderTargetDescriptor.h"
#include "Viewport.h"
#include "Color.h"
#include "WindowWin32.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include "Vec4.h"
#include "Mtx4.h"

// Backend Common
#include "DeviceContext.h"
#include "DeviceBase.h"
#include "DeviceTargetBase.h"

// Backend DIB
#include "RenderDeviceDIB.h"

// Backend Stencil
#include "RenderDeviceStencil.h"


#include "RenderResourceManager.h"

#include "Mesh.h"

#include "SlotArray.h"
#include "AssetRegistry.h"

namespace Render {

    template <typename Device>
    class Renderer {
    private:

        std::unique_ptr<Device> device;


        using Context = Device::Context;
        using ResourceManager = RenderResourceManager<Device>;

        using TargetHandle = ResourceManager::TargetHandle;

        ResourceManager resources;


        TargetHandle framebufferHandle;
        TargetHandle depthbufferHandle;

    public:
        Renderer(WindowWin32& window)
            : device(std::make_unique<Device>(window))
            , framebufferHandle(
                resources.targets.add(
                    RenderTargetDescriptor::FramebufferRGBA32bit(window.getClientWidth(), window.getClientHeight())
                )
            )
            , depthbufferHandle(
                resources.targets.add(
                    RenderTargetDescriptor::Depthbuffer24bit(window.getClientWidth(), window.getClientHeight())
                )
            )
        {
        }

        void renderFrame(uint32_t frameIndex)
        {
            if (0)
            {
                // zatim pouze pro testovani - pak to tu nebude
                using MeshHandle = RenderResourceManager<Device>::MeshHandle;

                MeshHandle MeshCubeHandler = resources.meshRegistry.add(Mesh::Cube(1.0f));
                MeshHandle MeshNetHandler = resources.meshRegistry.add(Mesh::Net(20));

                Mesh* msh = resources.meshRegistry.get(MeshCubeHandler);


                device->registerMesh(*msh);

            }

            auto& framebuffer = resources.targets.get(framebufferHandle);

            {

                uint32_t width = framebuffer.descriptor.width;
                uint32_t height = framebuffer.descriptor.height;
                float aspect = static_cast<float>(width) / static_cast<float>(height);

                auto ctx = device->createContext();
                ctx.frameIndex = frameIndex;
                ctx.clearColor = Color::Grayscale(0.4f);
                ctx.view = Mtx4::LookAt(
                    Vec4(5 + ctx.frameIndex / 100.0f, 5.0f, 5.0f, 1.0f),
                    Vec4(0, 0, 0, 1),
                    Vec4(0, 1, 0, 0)
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
            }

            device->present(framebuffer);

            //device-> // tady mi IDE nenapovida spravne...

        }

        void resize(uint32_t width, uint32_t height)
        {
            resources.targets.reset(framebufferHandle, RenderTargetDescriptor::FramebufferRGBA32bit(width, height));
            resources.targets.reset(depthbufferHandle, RenderTargetDescriptor::Depthbuffer24bit(width, height));
        }
    };


}