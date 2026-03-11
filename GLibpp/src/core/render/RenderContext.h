#pragma once

#include "RenderCommand.h"
#include "Renderer.h"
#include "utils/datastruct/Viewport.h"
#include "math/Mtx4.h"

class RenderContext {
public:

    RenderContext() : viewport(0,0,800,600), projection(Mtx4::identity()), modelview(Mtx4::identity()) {}

    Viewport viewport;
    Mtx4 projection;
    Mtx4 modelview;
};
