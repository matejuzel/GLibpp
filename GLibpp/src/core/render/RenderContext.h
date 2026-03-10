#pragma once

#include "RenderCommand.h"
#include "Renderer.h"
#include "utils/datastruct/Viewport.h"
#include "math/Mtx4.h"

class RenderContext {
public:

    RenderContext()
        : 
          projection(Mtx4::identity()),
          modelview( Mtx4::identity())
    {}

    Viewport viewport;
    Mtx4 projection;
    Mtx4 modelview;
};
