#pragma once

#include "RenderDeviceTraits.h"
#include "SlotArray.h"
#include "Mesh.h"

template <typename DerivedDevice, typename DerivedTarget>
class RenderDeviceBase
{
public:

    using Context = RenderContext<DerivedDevice, DerivedTarget>;
    using Target = DerivedTarget;
    using TargetHandle = SlotArray<DerivedTarget>::Handle;

    using VertexBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer3D;
    //using UVsBuffer      = typename DerivedDevice::GpuBuffer2D;
    //using GpuIndexBuffer = typename DerivedDevice::GpuIndexBuffer;

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

    void registerMesh(const Mesh& mesh) noexcept
    {
        static_cast<DerivedDevice*>(this)->registerMesh(mesh);
    }

    Context createContext() noexcept {
        return static_cast<DerivedDevice*>(this)->createContextImpl();
    }



    
};


