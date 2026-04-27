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

        using Context = DeviceContext<DerivedDevice>;
        using Target = DerivedTarget;

        using TargetRegistry = StableRegistry<Target>;
        using TargetHandle = typename TargetRegistry::Handle;
        static constexpr TargetHandle TARGET_INVALID = TargetRegistry::INVALID;


        using MeshHandle = uint32_t; // @todo

        // typy pro buffery
        using PositionBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer3D;
        using VectorBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer3D;
        using UVBuffer = typename DeviceTraits<DerivedDevice>::GpuBuffer2D;
        using IndexBuffer = typename DeviceTraits<DerivedDevice>::GpuIndexBuffer;

        DeviceBase(WindowWin32& window) : window(window) {}
        ~DeviceBase() = default;

		WindowWin32& getWindow() const noexcept { return window; }

        TargetHandle targetCreate(const RenderTargetDescriptor& descriptor) noexcept
        {
            return static_cast<DerivedDevice*>(this)->targetCreateImpl(descriptor);
		}

        TargetHandle targetResize(TargetHandle target_h, uint32_t width, uint32_t height) noexcept
        {
            return static_cast<DerivedDevice*>(this)->targetResizeImpl(target_h, width, height);
        }

        Target& targetGet(TargetHandle targetHandle)
        {
            return static_cast<DerivedDevice*>(this)->targetGetImpl(targetHandle);
		}
        
        void drawStaticTestMesh(const Context& ctx) noexcept
        {
            static_cast<DerivedDevice*>(this)->drawStaticTestMeshImpl(ctx);
        }

        void drawMeshEnqueue(const Context& ctx, MeshHandle meshHandle)
        {
            static_cast<DerivedDevice*>(this)->drawMeshEnqueueImpl(ctx);
        }

        void clear(const Context& ctx) noexcept
        {
            static_cast<DerivedDevice*>(this)->clearImpl(ctx);
        }

        void present(TargetHandle targetHandle) noexcept
        {
            static_cast<DerivedDevice*>(this)->presentImpl(targetHandle);
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




