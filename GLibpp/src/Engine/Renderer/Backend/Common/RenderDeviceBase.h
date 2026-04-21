#pragma once

#include "SlotArray.h"

template <typename DerivedDevice, typename DerivedTarget>
class RenderDeviceBase
{
public:

    using Context = RenderContext<DerivedDevice, DerivedTarget>;
    using Target = DerivedTarget;
    using TargetHandle = SlotArray<DerivedTarget>::Handle;

    WindowWin32& window;

    RenderDeviceBase(WindowWin32& window) : window(window) {}
    ~RenderDeviceBase() = default;

    void draw(const Context& ctx, DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->drawImpl(ctx, target);
    }

    void clear(const Context& ctx, DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->clearImpl(ctx, target);
    }

    void present(DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->presentImpl(target);
    }

    Context createContext() noexcept {
        return static_cast<DerivedDevice*>(this)->createContextImpl();
    }



    
};