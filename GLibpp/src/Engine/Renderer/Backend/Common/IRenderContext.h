#pragma once

#include "RenderState.h"
#include "IRenderTarget.h"

class IRenderDevice;
class RenderCommandList;
class Material;
class Mesh;

class IRenderContext {
protected:
    IRenderDevice& device;
    
    
    //Material& material;
    //Mesh& mesh;

public:
    IRenderContext(IRenderDevice& device) : device(device) {}
    virtual ~IRenderContext() = default;
    virtual void publish() = 0;

    Viewport viewport = { 0,0,0,0 };
    Mtx4 projection = Mtx4::Identity();
    Mtx4 view = Mtx4::Identity();
    IRenderTarget* target = nullptr;
	Color clearColor = { 0,0,0,255 };
};