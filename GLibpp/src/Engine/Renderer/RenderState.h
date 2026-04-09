#pragma once

#include "handles.h"
#include "Mtx4.h"
#include "Viewport.h"


struct RenderState {
    Viewport viewport = { 0,0,0,0 };
    Mtx4 projection = Mtx4::Identity();
    Mtx4 view = Mtx4::Identity();
    MaterialHandle material;
    MeshHandle mesh;
    DeviceTargetHandle target;
};