#pragma once

#include <memory>
#include "render/IRenderDevice.h"
#include "render/backendDIB/RenderTargetDIB.h"
#include "window/DIB/DIBFramebuffer.h"
#include "window/DIB/DIBRenderer.h"

class RenderDeviceDIB : public IRenderDevice {
private:
    std::unique_ptr<DIBFramebuffer> framebuffer;
    std::unique_ptr<DIBRenderer> dibRenderer;

    std::unique_ptr<IRenderContext> renderContext;

public:
    RenderDeviceDIB(WindowWin32* window, uint32_t width, uint32_t height, uint32_t bit) 
    {
        framebuffer = std::make_unique<DIBFramebuffer>(width, height, bit);
        framebuffer->init(window->getHwnd());

        dibRenderer = std::make_unique<DIBRenderer>(*framebuffer.get());
        dibRenderer->clear(0x00000000);

        framebuffer->present();
    }


    IRenderContext* getRenderContext() override
    {
        renderContext = nullptr;
        renderContext = std::make_unique<RenderContextDIB>();
        return renderContext.get();
	}

    std::shared_ptr<IRenderTarget> createRenderTarget(uint32_t width, uint32_t height) override
    {
		return std::make_shared<RenderTargetDIB>(framebuffer->getWidth(), framebuffer->getHeight());
	}

    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        // @todo
    }

    void setMatrixView(const Mtx4& view)
    {
        // @todo
    }

    void setMatrixProjection(const Mtx4& projection)
    {
        // @todo
    }

    MeshHandle registerMesh(const Mesh& mesh)
    {
        MeshHandle h;
        return h;
    }

    void drawMesh(MeshHandle handle, const Mtx4& modelMatrix, const Material& material)
    {
        // @todo
    }

    void clear(uint32_t color)
    {
        dibRenderer->clear(0x00000000);
    }

    void present()
    {
        framebuffer.get()->present();
    }

    void draw2dCircle(uint32_t x, uint32_t y, uint32_t size, uint32_t color)
    {
        dibRenderer->drawCircle(x, y, size, color);
    };

};
