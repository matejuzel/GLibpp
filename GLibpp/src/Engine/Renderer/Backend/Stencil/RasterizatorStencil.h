#pragma once

#include <cstdint>
#include <algorithm>
#include "RenderTargetBase.h"
#include "RenderTargetStencil.h"
#include <iostream>


class RasterizatorStencil {

public:

    static void inline drawLine(RenderTargetStencil& target, int x0, int y0, int x1, int y1, uint32_t color) noexcept
    {
        std::cout << "RasterizatorStencil::drawLine()" << std::endl;
    }

};