#pragma once

#include "IRenderContext.h"
#include "RenderCommandType.h"
#include "RenderCommandList.h"

class RenderContextDIB : public IRenderContext {
public:
    //RenderContextDIB(IRenderDevice& device) : IRenderContext(device) {}

    void renderCommandList(const RenderCommandList& cmds) override {
        for (const auto& cmd : cmds.getCommands()) {
            switch (cmd.type) {
            case RenderCommandType::BindMesh:
                state.mesh = cmd.data.bindMesh.mesh;
                state.material = cmd.data.bindMesh.material;
                break;
            case RenderCommandType::BindTarget:
                state.target = cmd.data.bindTarget.hwnd;
                break;
            case RenderCommandType::SetViewport:
                state.viewport = { cmd.data.setViewport.x, cmd.data.setViewport.y, cmd.data.setViewport.width, cmd.data.setViewport.height };
                break;
            case RenderCommandType::SetMatrixProjection:
                state.projection = cmd.data.setMatrixProjection.matrix;
                break;
            case RenderCommandType::SetMatrixView:
                state.view = cmd.data.setMatrixView.matrix;
                break;
            case RenderCommandType::Draw:
                device.drawMesh(*this);
                break;
            case RenderCommandType::Clear:
                device.clear(*this, cmd.data.clear.color);
                break;
            }
        }
    }

    void publish() override {
        // @todo: present / flush
    }
};