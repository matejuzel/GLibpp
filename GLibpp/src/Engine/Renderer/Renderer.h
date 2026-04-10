#pragma once

#include <cstdint>

#include "IRenderTarget.h"
#include "IRenderDevice.h"
#include "IRenderContext.h"
#include "RenderTargetDIB.h"
#include "RenderContextDIB.h"
#include "RenderDeviceDIB.h"
#include "RenderCommandList.h"
#include "RenderResourceManager.h"

class Renderer {
private:
    IRenderDevice& device;
    RenderResourceManager resources;
    uint32_t width = 0;
    uint32_t height = 0;

    DeviceTargetHandle framebufferBackIndex = { 0 };
    DeviceTargetHandle framebufferFrontIndex = { 0 };

public:
    Renderer(IRenderDevice& device, uint32_t width, uint32_t height)
        : device(device), width(width), height(height) {

        framebufferBackIndex = device.createTarget(RenderTargetDescriptor::Framebuffer(width, height));
		//framebufferFrontIndex = device.createRenderTarget(RenderTargetDescriptor::Framebuffer(width, height));
    }


    
    IRenderTarget& acquireFramebufferBack() {
        return device.getTarget(framebufferBackIndex);
	}



    void renderCommandList(const RenderCommandList& cmds, IRenderContext& ctx) {
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

        auto& target = acquireFramebufferBack();

        auto ctx = device.beginContext(target);
        
        renderCommandList(commandList, *ctx);
        
        
        ctx->publish();

        device.present(target);
    }
};