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
    struct Framebuffer {
        std::vector<uint32_t> pixels;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    // nahradi drivejsi std::array<Framebuffer,2> buffers_;
    DoubleBuffer<Framebuffer> buffers_;

public:
    std::unique_ptr<IRenderTarget> createRenderTarget(const RenderTargetDescriptor& descriptor) override {
        
        /* // @todo
        
        // alokujeme buffery pres writeBuffer()
        auto& fb = buffers_.writeBuffer(); // back buffer pro inicializaci
        fb.width = descriptor.width;
        fb.height = descriptor.height;
        fb.pixels.assign(static_cast<size_t>(fb.width) * fb.height, 0u);

        // take care: i druhy slot musi byt inicializovan
        auto& fb2 = buffers_.readBuffer(); // front slot
        fb2.width = fb.width;
        fb2.height = fb.height;
        fb2.pixels.assign(static_cast<size_t>(fb2.width) * fb2.height, 0u);
        */
        return std::make_unique<RenderTargetDIB>(descriptor);
    }

    // Acquire backbuffer vraci identifikator (zachovano stavajici API)
    DeviceTargetHandle acquireBackbuffer() override {
        return DeviceTargetHandle{ static_cast<uint32_t>(buffers_.backIndex()) };
    }

    // beginContext pouziva target (identifikator), context se bude odkazovat na tento handle
    std::unique_ptr<IRenderContext> beginContext() override {
        return std::make_unique<RenderContextDIB>(*this);
    }

    // Present: device bere zodpovednost za prehozeni (publishing) back->front
    void present(DeviceTargetHandle /*target*/) override {
        // jednoduse prehodime (writer jiz do back bufferu napsal a vola present)
        buffers_.publish();
    }

    void clear(IRenderContext& ctx, const Color& color) override {
        const auto& state = ctx.getState();
        // clear na back buffer - pouzijeme handle v state.target
        uint32_t idx = state.target.handle;
        if (static_cast<int>(idx) == buffers_.backIndex()) {
            auto& fb = buffers_.writeBuffer();
            uint32_t packed = (static_cast<uint32_t>(color.a) << 24) | (static_cast<uint32_t>(color.b) << 16) | (static_cast<uint32_t>(color.g) << 8) | (static_cast<uint32_t>(color.r));
            std::fill(fb.pixels.begin(), fb.pixels.end(), packed);
        }
        (void)state;
    }

    void drawMesh(IRenderContext& ctx) override {
        const auto& state = ctx.getState();
        // use state.mesh / state.material / state.viewport / state.target
        // @todo: software draw implementation
        (void)state;
    }
};