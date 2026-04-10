#pragma once

#include "IRenderContext.h"
#include "RenderCommandType.h"
#include "RenderCommandList.h"
#include "RenderDeviceDIB.h"

class RenderDeviceDIB;

class RenderContextDIB : public IRenderContext {
public:
    RenderContextDIB(IRenderDevice& device, IRenderTarget* targetDefault) : IRenderContext(device, targetDefault) {}

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

    void publish() override {

		presentToWindow();

        // @todo: present / flush
    }
};