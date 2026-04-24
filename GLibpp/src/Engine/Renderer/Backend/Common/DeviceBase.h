#pragma once

#include "StableRegistry.h"
#include "Mesh.h"

namespace Render {

    // trails - cpp koncept pro predavani informaci o typu (compile-time)
    // potreba, aby DeviceBase mohl deklarovat VertexBuffer (a dalsi) jako sablonu a kazdy backend tyto typy musel poskytovat
    template<typename Device>
    struct DeviceTraits;

    /** CLASS DeviceBase
     * CRTP — Curiously Recurring Template Pattern (v podstate compile-time polymorfizmus)
     * Base trida definuje rozhrani, ktere kazdy backend "DerivedDevice" musi implementovat
     */
    template <typename DerivedDevice, typename DerivedTarget>
    class DeviceBase
    {

    protected:
        WindowWin32& window;

    public:

        using Context = DeviceContext<DerivedDevice, DerivedTarget>;
        using Target = DerivedTarget;

        using TargetRegistry = StableRegistry<DerivedTarget>;
        using TargetHandle = typename TargetRegistry::Handle;
        static constexpr TargetHandle TARGET_INVALID = TargetRegistry::INVALID;

        StableRegistry<Target> targets;

        using MeshHandle = uint32_t; // @todo

        // typy pro buffery
        using PositionBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer3D;
        using VectorBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer3D;
        using UVBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer2D;
        using IndexBuffer = typename DeviceTraits<DerivedDevice>::GpuIndexBuffer;

        DeviceBase(WindowWin32& window) : window(window) {}
        ~DeviceBase() = default;

        
        void drawStaticTestMesh(const Context& ctx) noexcept
        {
            
            if (targets.isValid(ctx.framebufferHandle)) {
                // vynutime kontrolu validnosti targetu uz tady v Base
                static_cast<DerivedDevice*>(this)->drawStaticTestMeshImpl(ctx);
            }
        }

        void drawMeshEnqueue(const Context& ctx, MeshHandle meshHandle)
        {
            if (targets.isValid(ctx.framebufferHandle)) {
                static_cast<DerivedDevice*>(this)->drawMeshEnqueueImpl(ctx);
            }
        }

        void clear(const Context& ctx) noexcept
        {
            if (targets.isValid(ctx.framebufferHandle)) {
                static_cast<DerivedDevice*>(this)->clearImpl(ctx);
            }
        }

        void present(TargetHandle targetHandle) noexcept
        {
            if (targets.isValid(targetHandle)) {
                static_cast<DerivedDevice*>(this)->presentImpl(targetHandle);
            }
        }

        void registerMesh(const Mesh& mesh) noexcept
        {
            static_cast<DerivedDevice*>(this)->registerMeshImpl(mesh);
        }

        Context createContext() noexcept {
            return static_cast<DerivedDevice*>(this)->createContextImpl();
        }


    };

    /*
    template <typename DerivedDevice, typename DerivedTarget>
    using TargetRegistry = StableRegistry<DerivedTarget>;

    template <typename DerivedDevice, typename DerivedTarget>
    using TargetHandle = typename DeviceBase<DerivedDevice, DerivedTarget>::TargetHandle;

    template <typename DerivedDevice, typename DerivedTarget>
    static constexpr TargetHandle TARGET_INVALID = TargetRegistry::INVALID;
    */
}




