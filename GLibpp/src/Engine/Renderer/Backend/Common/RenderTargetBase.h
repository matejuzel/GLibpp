#pragma once

#include "RenderContext.h"

template<typename Device, typename DerivedTarget>
class RenderTargetBase
{
public:

    RenderTargetDescriptor descriptor;

    RenderTargetBase() = default;
    RenderTargetBase(const RenderTargetDescriptor& descriptor) :descriptor(descriptor) {}
    ~RenderTargetBase() = default;
};