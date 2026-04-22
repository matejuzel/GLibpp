#pragma once

namespace Render {


}

/*
#include "RenderTargetStencil.h"
#include "RenderDeviceBase.h"
#include "RenderTargetDescriptor.h"
#include "WindowWin32.h"

class RenderDeviceStencil;

namespace internal {
    using RenderDeviceStencilBase = RenderDeviceBase<RenderDeviceStencil, RenderTargetStencil>;
};


class RenderDeviceStencil : public internal::RenderDeviceStencilBase
{
private:

    using Self = RenderDeviceStencil;
    using Base = internal::RenderDeviceStencilBase;

public:

    using Target = RenderTargetStencil;
    using Context = RenderContext<Self, Target>;

    RenderDeviceStencil(WindowWin32& window) : Base(window) {}

    void drawImpl(const Context& ctx, Target& target) noexcept
    {
        // @todo
    }

    void clearImpl(const Context& ctx, Target& target) noexcept
    {
        // @todo
    }

    void presentImpl(Context& ctx, const Target& target) noexcept
    {
        // @todo
    }

    std::unique_ptr<Target> createTargetImpl(const RenderTargetDescriptor& descriptor) noexcept
    {
        return std::make_unique<Target>(descriptor);
    }
};
*/