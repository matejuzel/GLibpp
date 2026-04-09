#pragma once 

#include "RenderCommandStructs.h"

union RenderCommandData {
    RCMD_SetViewport setViewport;
    RCMD_SetMatrixProjection setMatrixProjection;
    RCMD_SetMatrixView setMatrixView;
    RCMD_BindMesh bindMesh;
    RCMD_BindTarget bindTarget;
    RCMD_Draw draw;
    RCMD_Clear clear;
    RenderCommandData() {}
    ~RenderCommandData() {}
};

