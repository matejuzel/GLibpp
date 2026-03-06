#pragma once

#include "RenderCommand.h"
#include "Renderer.h"

class RenderContext {
public:

    RenderContext()
        : 
          projection(Mtx4::identity()),
          modelview( Mtx4::identity())
    {}

    //Viewport vp;
    Mtx4 projection;
    Mtx4 modelview;
};
