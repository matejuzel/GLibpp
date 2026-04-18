#pragma once

#include "RenderTargetBase.h"
#include "RenderTargetDescriptor.h"
class RenderDeviceStencil; // forward
class RenderTargetStencil; // forward

namespace internal {
    using RenderTargetStencilBase = RenderTargetBase<RenderDeviceStencil, RenderTargetStencil>;
};

class RenderTargetStencil : public internal::RenderTargetStencilBase
{
private:
    using Self = RenderTargetStencil;
    using Base = RenderTargetBase<RenderDeviceStencil, Self>;

public:

    RenderTargetStencil(const RenderTargetDescriptor& descriptor)
        : Base(descriptor)
    {}

    ~RenderTargetStencil() {}

};