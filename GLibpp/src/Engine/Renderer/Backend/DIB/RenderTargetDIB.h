#pragma once

#include "IRenderTarget.h"
#include "RenderTargetDescriptor.h"

class RenderTargetDIB : public IRenderTarget {
public:
    RenderTargetDIB(const RenderTargetDescriptor& desc) : IRenderTarget(desc) {}
    // @todo: actual DIB data
};