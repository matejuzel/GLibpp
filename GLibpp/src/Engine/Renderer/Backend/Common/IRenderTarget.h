#pragma once

#include "RenderTargetDescriptor.h"

class IRenderTarget {
private:
    RenderTargetDescriptor descriptor;
public:
    IRenderTarget(const RenderTargetDescriptor& descriptor) : descriptor(descriptor) {}
    virtual ~IRenderTarget() = default;
};