#pragma once
/*
#include <memory>
#include "RenderContext.h"
#include "IRenderTarget.h"
#include "DeviceTargetHandle.h"
#include "Color.h"
#include "RenderTargetDescriptor.h"

template <typename Derived>
class IRenderDevice {
public:

	IRenderDevice() = default;
	~IRenderDevice() = default;

    RenderContext<Derived> beginContext()
    {
        return static_cast<Derived*>(this)->beginContextImpl();
    }
    
    DeviceTargetHandle createTarget(const RenderTargetDescriptor& descriptor) 
    {
        return static_cast<Derived*>(this)->createTargetImpl(descriptor);
	}

	IRenderTarget& getTarget(const DeviceTargetHandle& handle) 
    {
        return static_cast<Derived*>(this)->getTargetImpl(handle);
	}

    void clear(const RenderContext<Derived>& ctx) {
        static_cast<Derived*>(this)->clearImpl(ctx);
    }

    void drawMesh(const RenderContext<Derived>& ctx)
    {
        static_cast<Derived*>(this)->drawMeshImpl(ctx);
	}
    
    void present(const RenderContext<Derived>& ctx)
    {
        static_cast<Derived*>(this)->presentImpl(ctx);
    }
};
*/