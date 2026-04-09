#pragma once

#include "RenderState.h"

class IRenderDevice;
class RenderCommandList;

class IRenderContext {
protected:
    IRenderDevice& device;
    RenderState state;

public:
    IRenderContext(IRenderDevice& device) : device(device) {}
    const RenderState& getState() const noexcept { return state; }
    virtual ~IRenderContext() = default;
    virtual void renderCommandList(const RenderCommandList& cmds) = 0;
    virtual void publish() = 0;
};