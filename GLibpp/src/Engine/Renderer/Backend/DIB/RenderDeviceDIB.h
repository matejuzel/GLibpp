#pragma once

#include "IRenderDevice.h"
#include "IRenderTarget.h"
#include "IRenderContext.h"
#include "RenderTargetDescriptor.h"
#include "RenderTargetDIB.h"
#include "RenderContextDIB.h"
#include "DoubleBuffer.h"
#include <vector>

class RenderDeviceDIB : public IRenderDevice {

private:
    
	std::vector<RenderTargetDIB> renderTargets_;

	DeviceTargetHandle backbufferHandle_ = { 0 }; // @todo - handle pro backbuffer, prozatim pevne 0

public:
    DeviceTargetHandle createRenderTarget(const RenderTargetDescriptor& descriptor) override 
    {
        size_t index = renderTargets_.size();
		renderTargets_.emplace_back(descriptor);
		return DeviceTargetHandle{ static_cast<uint32_t>(index) };
    }

    // Acquire backbuffer vraci identifikator (zachovano stavajici API)
    DeviceTargetHandle acquireBackbuffer() override 
    {
		return DeviceTargetHandle{ 0 }; // @todo - vracet handle pro backbuffer
    }

    std::unique_ptr<IRenderContext> beginContext(IRenderTarget& target) override
    {
        auto a = std::make_unique<RenderContextDIB>(*this, &target);
        return a;
    }

    void present(IRenderTarget& target) override 
    {
        // pak prehodi front a back buffer
    }

    void clear(IRenderContext& ctx, IRenderTarget& target) override
    {
        uint32_t color = ctx.clearColor.toRGBA();
        
        // @todo

    }

    void drawMesh(IRenderContext& ctx) override 
    {

        class MeshInstance {
            // @todo
			Mtx4 transformation;
        public:
			const Mtx4& getTransfomation() const { return transformation; }
        };

        MeshInstance mesh;

        ctx.target;
        Mtx4 mvp = mesh.getTransfomation() * ctx.projection * ctx.view;

		auto* dib = static_cast<RenderTargetDIB*>(ctx.target);
        dib->putPixel(10, 10, 0xff0000ff);
    }

    
};