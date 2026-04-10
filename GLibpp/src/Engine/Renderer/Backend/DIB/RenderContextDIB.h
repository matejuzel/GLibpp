#pragma once

#include "IRenderContext.h"
#include "RenderCommandType.h"
#include "RenderCommandList.h"

class RenderContextDIB : public IRenderContext {
public:
    RenderContextDIB(IRenderDevice& device, IRenderTarget* targetDefault) : IRenderContext(device, targetDefault) {}

    

    void publish() override {
        // @todo: present / flush
    }
};