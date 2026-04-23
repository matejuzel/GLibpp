#pragma once

#include "SlotArray.h"
#include "Mesh.h"

namespace Render {

    // trails - cpp koncept pro predavani informaci o typu (compile-time)
    // potreba, aby RenderDeviceBase mohl deklarovat VertexBuffer (a dalsi) jako sablonu a kazdy backend tyto typy musel poskytovat
    template<typename Device>
    struct DeviceTraits;

    /** CLASS DeviceBase
     * CRTP — Curiously Recurring Template Pattern (v podstate compile-time polymorfizmus)
     * Base trida definuje rozhrani, ktere kazdy backend "DerivedDevice" musi implementovat
     */
    template <typename DerivedDevice, typename DerivedTarget>
    class RenderDeviceBase
    {

    protected:
        WindowWin32& window;

    public:

        using Context = RenderContext<DerivedDevice, DerivedTarget>;
        using Target = DerivedTarget;
        using TargetHandle = typename SlotArray<DerivedTarget>::Handle;

        // typy pro buffery
        using PositionBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer3D;
        using VectorBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer3D;
        using UVBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer2D;
        using IndexBuffer = typename DeviceTraits<DerivedDevice>::GpuIndexBuffer;

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
            static_cast<DerivedDevice*>(this)->registerMeshImpl(mesh);
        }

        Context createContext() noexcept {
            return static_cast<DerivedDevice*>(this)->createContextImpl();
        }


    };

}




