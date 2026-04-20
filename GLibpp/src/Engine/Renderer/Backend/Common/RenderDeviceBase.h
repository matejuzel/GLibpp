#pragma once

#include "SlotArray.h"

template <typename DerivedDevice, typename DerivedTarget>
class RenderDeviceBase
{
public:


    using TargetHandle = SlotArray<DerivedTarget>::Handle;


    using Context = RenderContext<DerivedDevice, DerivedTarget>;
    WindowWin32& window;

    RenderDeviceBase(WindowWin32& window) : window(window) {}
    //~RenderDeviceBase() = default;

    void draw(const Context& ctx) noexcept
    {
        static_cast<DerivedDevice*>(this)->drawImpl(ctx);
    }

    void clear(const Context& ctx) noexcept
    {
        static_cast<DerivedDevice*>(this)->clearImpl(ctx);
    }

    void present(Context& ctx) noexcept
    {
        static_cast<DerivedDevice*>(this)->presentImpl(ctx);
    }




    std::unique_ptr<DerivedTarget> createTarget(const RenderTargetDescriptor& descriptor) noexcept
    {
        return static_cast<DerivedDevice*>(this)->createTargetImpl(descriptor);
    }
};