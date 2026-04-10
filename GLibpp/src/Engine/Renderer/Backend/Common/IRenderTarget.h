#pragma once

#include "RenderTargetDescriptor.h"

class IRenderTarget {
protected:
    RenderTargetDescriptor descriptor;
public:
    IRenderTarget(const RenderTargetDescriptor& descriptor) : descriptor(descriptor) {}
    virtual ~IRenderTarget() = default;
	const RenderTargetDescriptor& getDescriptor() const { return descriptor; }
};