#include "RenderDeviceDIB.h"
#include "RenderContextDIB.h"
#include <memory>

std::unique_ptr<IRenderContext> RenderDeviceDIB::beginContext(IRenderTarget& target)
{
    // teï už je RenderContextDIB kompletnì definovaný — konverze unique_ptr<RenderContextDIB> -> unique_ptr<IRenderContext> funguje
    return std::make_unique<RenderContextDIB>(*this, &target);
}