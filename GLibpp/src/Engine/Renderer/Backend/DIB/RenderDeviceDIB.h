#pragma once

//#include "DoubleBuffer.h"
#include "IRenderDevice.h"
#include "IRenderTarget.h"
#include "IRenderContext.h"
#include "RenderTargetDescriptor.h"
#include "RenderTargetDIB.h"
#include <windows.h>
#include <vector>

class RenderContextDIB;

class RenderDeviceDIB : public IRenderDevice {

private:
    
	std::vector<std::unique_ptr<RenderTargetDIB>> renderTargets_;
	HWND hwnd_ = nullptr;

public:

	RenderDeviceDIB(HWND hwnd) : IRenderDevice(), hwnd_(hwnd)
    {}

	void setHwnd(HWND hwnd) { hwnd_ = hwnd; }

    HWND getHwnd() const { return hwnd_; }

    DeviceTargetHandle createTarget(const RenderTargetDescriptor& descriptor) override 
    {
        size_t index = renderTargets_.size();
		renderTargets_.push_back(std::make_unique<RenderTargetDIB>(descriptor));
		return DeviceTargetHandle{ static_cast<uint32_t>(index) };
    }

    IRenderTarget& getTarget(const DeviceTargetHandle& handle) override {
        if (handle.handle >= renderTargets_.size()) {
            throw std::runtime_error("Invalid RenderTarget handle: " + std::to_string(handle.handle));
		}
		return *renderTargets_[handle.handle];
    };


    // deklarace pouze — implementace pøesunuta do .cpp, aby byla dostupná úplná definice RenderContextDIB
    std::unique_ptr<IRenderContext> beginContext(IRenderTarget& target) override;

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
        dib->putPixel(11, 10, 0xff0000ff);
        dib->putPixel(12, 10, 0xff0000ff);
        dib->putPixel(13, 10, 0xff0000ff);
        dib->putPixel(14, 10, 0xff0000ff);
        dib->putPixel(10, 12, 0xff0000ff);
    }

    
};