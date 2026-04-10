#pragma once

#include <memory>
#include "DeviceTargetHandle.h"
#include "Color.h"
#include "IRenderContext.h"
#include "IRenderTarget.h"
#include "RenderTargetDescriptor.h"


class IRenderDevice;
class IRenderDevice {
public:
    virtual DeviceTargetHandle acquireBackbuffer() = 0;
    virtual void present(IRenderTarget& target) = 0;
    virtual std::unique_ptr<IRenderContext> beginContext() = 0;
    virtual DeviceTargetHandle createRenderTarget(const RenderTargetDescriptor& descriptor) = 0;

    virtual void drawMesh(IRenderContext& ctx) = 0;
    virtual void clear(IRenderContext& ctx, IRenderTarget& target) = 0;
};
