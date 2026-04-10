#pragma once

#include "IRenderContext.h"
#include "RenderCommandType.h"
#include "RenderCommandList.h"

class RenderContextDIB : public IRenderContext {
public:
    RenderContextDIB(IRenderDevice& device, IRenderTarget* targetDefault) : IRenderContext(device, targetDefault) {}

    

    void presentToWindow()
    {

        
		HWND hwnd_ = static_cast<RenderDeviceDIB&>(device).getHwnd();

        HDC windowDC = GetDC(hwnd_);

        BitBlt(
            windowDC,
            0, 0,                          // cílová pozice v oknì
            descriptor.width,
            descriptor.height,
            memDC,                         // zdrojový DC (DIB)
            0, 0,                          // pozice v DIB
            SRCCOPY
        );

        ReleaseDC(hwnd, windowDC);
    }

    void publish() override {



        // @todo: present / flush
    }
};