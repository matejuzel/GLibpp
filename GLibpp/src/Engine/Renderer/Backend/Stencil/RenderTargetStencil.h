#pragma once

#include "RenderTargetDescriptor.h"
class RenderDeviceStencil; // forward

class RenderTargetStencil : public RenderTargetBase<RenderDeviceStencil, RenderTargetStencil>
{
public:

    RenderTargetStencil(const RenderTargetDescriptor& descriptor)
        : RenderTargetBase<RenderDeviceStencil, RenderTargetStencil>(descriptor)
    {}

    ~RenderTargetStencil() {}

};