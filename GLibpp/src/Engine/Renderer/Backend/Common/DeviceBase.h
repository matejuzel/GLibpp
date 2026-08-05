#pragma once


#include "StableRegistry.h"
#include "Mesh.h"
#include "ResourceHandles.h"

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
        
        // handle-based kresleni - backend cerpa z vlastni residency (identita = MeshHandle);
        // nevalidni/nezaregistrovany handle se tise preskoci
        void drawMesh(const Context& ctx, MeshHandle h, const Mtx4& transform, const Color& color = Color::Grayscale(0.3f), bool wiredFlag = false) noexcept
        {
            static_cast<DerivedDevice*>(this)->drawMeshImpl(ctx, h, transform, color, wiredFlag);
        }

        void drawAxis(const Context& ctx, const Mtx4& transform)
        {
            static_cast<DerivedDevice*>(this)->drawAxisImpl(ctx, transform);
        }

        void clear(const Context& ctx) noexcept
        {
            static_cast<DerivedDevice*>(this)->clearImpl(ctx);
        }

        void present(TargetHandle targetHandle) noexcept
        {
            static_cast<DerivedDevice*>(this)->presentImpl(targetHandle);
        }

        // registrace geometrie v backendu - identita = MeshHandle (razi ho ResourceManager)
        // backend si pod handlem uklada vlastni residency (kopie ve velkych polich, offsety, VBO, ...)
        // vola se z upload walku na zacatku runLoop (render vlakno - u GL tu bude aktivni context)
        void meshRegister(MeshHandle h, const Mesh& mesh) noexcept
        {
            static_cast<DerivedDevice*>(this)->meshRegisterImpl(h, mesh);
        }

        // notifikace o zmene dat meshe (dynamicke meshe, napr. GridWave) - backend si obnovi svou kopii
        void meshUpdate(MeshHandle h, const Mesh& mesh) noexcept
        {
            static_cast<DerivedDevice*>(this)->meshUpdateImpl(h, mesh);
        }

        Context createContext() noexcept {
            return static_cast<DerivedDevice*>(this)->createContextImpl();
        }

    protected:

        // default no-op - backend, ktery zadnou vlastni residency nepotrebuje, nic neimplementuje
        void meshRegisterImpl(MeshHandle, const Mesh&) noexcept {}
        void meshUpdateImpl(MeshHandle, const Mesh&) noexcept {}

    };


}




