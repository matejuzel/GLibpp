#pragma once

#include "RenderTargetDescriptor.h"

template<typename RenderDevice>
class IRenderTarget 
{
protected:
    RenderTargetDescriptor descriptor;
public:
    IRenderTarget(const RenderTargetDescriptor& descriptor) 
        : descriptor(descriptor) 
    {}

    ~IRenderTarget() = default;

	const RenderTargetDescriptor& getDescriptor() const {
        return descriptor; 
    }

};