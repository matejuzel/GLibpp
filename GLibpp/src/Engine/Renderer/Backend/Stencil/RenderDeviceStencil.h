#pragma once

#include "RenderDeviceBase.h"
#include "RasterizatorStencil.h"
#include "RenderTargetDescriptor.h"
#include <windows.h>

class RenderTargetStencil;


class RenderDeviceStencil : public RenderDeviceBase<RenderDeviceStencil, RenderTargetStencil>
{
public:

    using Device = RenderDeviceStencil;
    using Target = RenderTargetStencil;
    using Context = RenderContext<Device, Target>;
    using Backend = RenderDeviceBase<Device, Target>;

    RenderDeviceStencil(HWND hwnd) {}

    //protected:
    void drawImpl(const Context& ctx, Target& target) noexcept
    {
        RasterizatorStencil::drawLine(target, 0, 0, 0, 0, 0);
    }

    void clearImpl(const Context& ctx, Target& target) noexcept
    {
    }

    void presentImpl(Context& ctx, const Target& target) noexcept
    {
    }

    std::unique_ptr<RenderTargetStencil> createTargetImpl(const RenderTargetDescriptor& descriptor) noexcept
    {
        return std::make_unique<RenderTargetStencil>(descriptor);
    }
};