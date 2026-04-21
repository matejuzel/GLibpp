#pragma once

#include "SlotArray.h"
#include "Mesh.h"

// trails - cpp koncept pro predavani informaci o typu (compile-time)
// potreba, aby RenderDeviceBase mohl deklarovat VertexBuffer (a dalsi) jako sablonu a kazdy backend tyto typy musel poskytovat
template<typename Device>
struct DeviceTraits;

// pouzit CRTP — Curiously Recurring Template Pattern
// v podstate compile-time polymorfizmus
// RenderDeviceBase definuje rozhrani, ktere kazdy backend "DerivedDevice" musi implementovat
template <typename DerivedDevice, typename DerivedTarget>
class RenderDeviceBase
{
public:

    using Context      = RenderContext<DerivedDevice, DerivedTarget>;
    using Target       = DerivedTarget;
    using TargetHandle = SlotArray<DerivedTarget>::Handle;

    using VertexBuffer   = typename DeviceTraits<DerivedDevice>::GpuBuffer3D;
    using UVsBuffer      = typename DeviceTraits<DerivedDevice>::GpuBuffer2D;
    using GpuIndexBuffer = typename DeviceTraits<DerivedDevice>::GpuIndexBuffer;

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


