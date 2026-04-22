#pragma once


namespace Render {


}


/*
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
    using Base = internal::RenderTargetStencilBase;

public:

    RenderTargetStencil() = default;
    ~RenderTargetStencil() {}

    RenderTargetStencil(const RenderTargetDescriptor& descriptor)
        : Base(descriptor)
    {}

};
*/