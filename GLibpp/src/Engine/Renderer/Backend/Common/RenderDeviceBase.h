#pragma once

template <typename DerivedDevice, typename DerivedTarget>
class RenderDeviceBase
{
public:

    using Context = RenderContext<DerivedDevice, DerivedTarget>;
    WindowWin32& window;

    RenderDeviceBase(WindowWin32& window) : window(window) {}
    //~RenderDeviceBase() = default;

    void draw(const Context& ctx, DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->drawImpl(ctx, target);
    }

    void clear(const Context& ctx, DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->clearImpl(ctx, target);
    }

    std::unique_ptr<DerivedTarget> createTarget(const RenderTargetDescriptor& descriptor) noexcept
    {
        return static_cast<DerivedDevice*>(this)->createTargetImpl(descriptor);
    }

    void present(Context& ctx, const DerivedTarget& target) noexcept
    {
        static_cast<DerivedDevice*>(this)->presentImpl(ctx, target);
    }
};