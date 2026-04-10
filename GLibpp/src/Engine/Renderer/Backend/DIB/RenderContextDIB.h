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

		RenderDeviceDIB& deviceDIB = static_cast<RenderDeviceDIB&>(device);
		RenderTargetDIB& targetDIB = static_cast<RenderTargetDIB&>(*target);

		HWND hwnd_ = deviceDIB.getHwnd();
        uint32_t width = target->getDescriptor().width;
        uint32_t height = target->getDescriptor().height;

        HDC windowDC = GetDC(hwnd_);


        BitBlt(
            windowDC,
            0, 0,                          // cílová pozice v oknì
            width,
            height,
            targetDIB.getDC(),                         // zdrojový DC (DIB)
            0, 0,                          // pozice v DIB
            SRCCOPY
        );

        ReleaseDC(hwnd_, windowDC);
    }

    void publish() override {

		presentToWindow();

        // @todo: present / flush
    }
};