#pragma once

/*
//#include "DoubleBuffer.h"
#include "IRenderDevice.h"
#include "IRenderTarget.h"
#include "RenderTargetDescriptor.h"
#include "RenderTargetDIB.h"
#include <windows.h>
#include <vector>

//class RenderContextDIB;

class RenderDeviceDIB : public IRenderDevice<RenderDeviceDIB> {

private:
    
	std::vector<std::unique_ptr<RenderTargetDIB>> renderTargets_;
	HWND hwnd_ = nullptr;

public:

	RenderDeviceDIB(HWND hwnd) : IRenderDevice(), hwnd_(hwnd) {}
    HWND getHwnd() const { return hwnd_; }

    DeviceTargetHandle createTargetImpl(const RenderTargetDescriptor& descriptor) 
    {
        size_t index = renderTargets_.size();
		renderTargets_.push_back(std::make_unique<RenderTargetDIB>(descriptor));
		return DeviceTargetHandle{ static_cast<uint32_t>(index) };
    }

protected:

    // deklarace pouze — implementace pøesunuta do .cpp, aby byla dostupná úplná definice RenderContextDIB
    RenderContext<RenderDeviceDIB> beginContextImpl() {
        return RenderContext<RenderDeviceDIB>();
    }




    IRenderTarget& getTargetImpl(const DeviceTargetHandle& handle) {
        if (handle.handle >= renderTargets_.size()) {
            throw std::runtime_error("Invalid RenderTarget handle: " + std::to_string(handle.handle));
		}
		return *renderTargets_[handle.handle];
    };

    void presentImpl(IRenderTarget& target) 
    {
        // pak prehodi front a back buffer
    }


    void clearImpl(const RenderContext<RenderDeviceDIB>& ctx)
    {
        uint32_t color = ctx.clearColor.toRGBA();
        
        std::fill_n(targetDib->getFramebuffer(), target.getDescriptor().width * target.getDescriptor().height, color);

		// dalo by se pouzit SSE (SIMD) pro rychlejší vyplnìní, ale pro jednoduchost a pøehlednost teï použijeme std::fill_n

    }

    void drawMeshImpl(IRenderContext& ctx) 
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


    void presentToWindow()
    {
        HWND hwnd_ = (static_cast<RenderDeviceDIB&>(device)).getHwnd();
        HDC windowDC = GetDC(hwnd_);

        BitBlt(
            windowDC,
            0, 0,
            target->getDescriptor().width, target->getDescriptor().height,
            (static_cast<RenderTargetDIB&>(*target)).getDC(),
            0, 0,
            SRCCOPY
        );

        ReleaseDC(hwnd_, windowDC);
    }
    

    void drawPrimitivePixel(RenderContext<RenderDeviceDIB>& ctx)
    {
        auto a = static_cast<RenderContext<RenderDeviceDIB>>(ctx.target);
    }

};*/