#pragma once
#include <memory>

#include "IRenderDevice.h"
#include "WindowWin32.h"
#include "Renderer.h"
#include "RenderCommandList.h"
#include "RenderDeviceDIB.h"
#include "Mtx4.h"

class App {
private:
    std::unique_ptr<IRenderDevice> device;
    WindowWin32 window;
    Renderer renderer;

public:
    App()
        : device(std::make_unique<RenderDeviceDIB>())
        , window(800, 600, false)
        , renderer(*device, window.getClientWidth(), window.getClientHeight())
    {
    }

    void runRenderingLoop()
    {
        // create render target using descriptor helper
        auto target = device->createRenderTarget(RenderTargetDescriptor::Framebuffer(window.getClientWidth(), window.getClientHeight()));

        renderer.runLoop(
            RenderCommandList::Create()
            .clear({ 0,0,0,255 })
            .setViewport(0, 0, window.getClientWidth(), window.getClientHeight())
            .setMatrixProjection(Mtx4::Perspective(
                45.0f,
                window.getClientWidth() / (float)window.getClientHeight(),
                0.01f,
                1000.0f
            ))
            .setMatrixView(Mtx4::LookAt(
                Vec4(1.0f, 1.0f, 1.0f, 0.0f),
                Vec4(0.0f, 0.0f, 0.0f, 0.0f),
                Vec4(0.0f, 1.0f, 0.0f, 1.0f)
            ))
            .bindMesh(MeshHandle{ 1 }, MaterialHandle{ 1 })
            .draw()
            .bindMesh(MeshHandle{ 1 }, MaterialHandle{ 1 })
            .draw()
            .setMatrixView(Mtx4::Identity())
            .bindMesh(MeshHandle{ 2 }, MaterialHandle{ 2 })
            .draw()
        );
    }
};