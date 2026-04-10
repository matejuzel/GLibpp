#pragma once

#include <memory>
#include "DeviceTargetHandle.h"
#include "Color.h"
#include "IRenderContext.h"
#include "IRenderTarget.h"
#include "RenderTargetDescriptor.h"
//#include <windows.h>


class IRenderDevice;
class IRenderDevice {
public:
    virtual std::unique_ptr<IRenderContext> beginContext(IRenderTarget& target) = 0;
    virtual DeviceTargetHandle createTarget(const RenderTargetDescriptor& descriptor) = 0;
	virtual IRenderTarget& getTarget(const DeviceTargetHandle& handle) = 0;

    virtual void drawMesh(IRenderContext& ctx) = 0;
    virtual void clear(IRenderContext& ctx, IRenderTarget& target) = 0;

    virtual void present(IRenderTarget& target) = 0;

    //virtual HWND getHwnd() const = 0;
};
