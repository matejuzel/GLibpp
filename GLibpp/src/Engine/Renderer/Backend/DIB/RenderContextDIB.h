#pragma once

#include "IRenderContext.h"
#include "RenderCommandType.h"
#include "RenderCommandList.h"

class RenderContextDIB : public IRenderContext {
public:
    RenderContextDIB(IRenderDevice& device) : IRenderContext(device) {}

    

    void publish() override {
        // @todo: present / flush
    }
};