#pragma once

#include <cstdint>

/*
#include "IRenderTarget.h"
#include "IRenderDevice.h"
#include "RenderTargetDIB.h"
#include "RenderDeviceDIB.h"
#include "RenderCommandList.h"
#include "RenderResourceManager.h"
*/

#include "RenderTargetDescriptor.h"
#include "RenderContext.h"

#include <windows.h>
#include <memory>



template <typename Derived>
class RenderDeviceBase
{
public:

    void draw(const RenderContext<Derived>& ctx) noexcept
    {
        static_cast<Derived*>(this)->drawImpl(ctx);
    }

    void clear(const RenderContext<Derived>& ctx) noexcept
    {
        static_cast<Derived*>(this)->clearImpl(ctx);
    }

    void createTarget(const RenderTargetDescriptor &descriptor) noexcept
    {
        static_cast<Derived*>(this)->createTargetImpl(descriptor);
    }
};

class RenderDeviceDIB : public RenderDeviceBase<RenderDeviceDIB>
{
public:
    void drawImpl(const RenderContext<RenderDeviceDIB>& ctx) noexcept
    {
        // @todo
    }

    void clearImpl(const RenderContext<RenderDeviceDIB>& ctx) noexcept
    {
        // @todo
    }

    void createTargetImpl(const RenderTargetDescriptor& descriptor) noexcept
    {
        // @todo
    }
};



template <typename Backend>
class Renderer {
private:
    std::unique_ptr<Backend> device = nullptr;

public:
    
    Renderer()
        : device(std::make_unique<Backend>())
    {

        RenderContext<Backend> ctx = {};
        device->clear(ctx);
        device->draw(ctx);
    }

    /*
    Renderer(RenderDeviceDIB& device, uint32_t width, uint32_t height)
        : device(device)
        , width(width)
        , height(height) 
    {
        framebufferBackIndex = device.createTarget(RenderTargetDescriptor::Framebuffer(width, height));
    }
    
    IRenderTarget& acquireFramebufferBack() {
        return device.getTarget(framebufferBackIndex);
	}

    void executeRenderCommands(const RenderCommandList& cmds, RenderDeviceDIB& ctx) {
        for (const auto& cmd : cmds.getCommands()) {
            switch (cmd.type) {
            case RenderCommandType::BindMesh:
                //ctx.state.mesh = cmd.data.bindMesh.mesh_h;
                //ctx.state.material = cmd.data.bindMesh.material_h;
                break;

            case RenderCommandType::BindTarget:

                
                //ctx.target = &getRenderTarget(cmd.data.bindTarget.target_h);
				ctx.target = &device.getTarget(cmd.data.bindTarget.target_h);
                break;

            case RenderCommandType::SetViewport:
                ctx.viewport = {
                    cmd.data.setViewport.x,
                    cmd.data.setViewport.y,
                    cmd.data.setViewport.width,
                    cmd.data.setViewport.height
                };
                break;

            case RenderCommandType::SetMatrixProjection:
                ctx.projection = Mtx4(cmd.data.setMatrixProjection.matrixData);
                break;

            case RenderCommandType::SetMatrixView:
                ctx.view = Mtx4(cmd.data.setMatrixView.matrixData);
                break;

            case RenderCommandType::Draw:
                device.drawMesh(ctx);
                break;

            case RenderCommandType::Clear:
				ctx.clearColor = cmd.data.clear.color;
                device.clear(ctx, *ctx.target);
                break;

            }
        }
    }

    void runLoop(const RenderCommandList& commandList) {

        HWND hwnd;
        auto device = std::make_unique<RenderDeviceDIB>(hwnd);

        bool running = true;
        while (running)
        {

            RenderContext<RenderDeviceDIB> ctx = 

            auto ctx = device.beginContext(); // zazada device o novy kontext

            executeRenderCommands(commandList, ctx);

            ctx.publish(); // swapne front/back buffer
            ctx.end(); // ukonci kontext
        }

        


        





        auto ctx = device.beginContext(target);
        
        renderCommandList(commandList, *ctx);
        
        
        ctx->publish();

        device.present(target);
    }
    */
};