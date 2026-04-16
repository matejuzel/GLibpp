#pragma once

#include "RenderState.h"
#include "IRenderTarget.h"
#include "Color.h"
class Material;
class MeshInstance;

template<typename Device>
class RenderContext {

public:
    RenderContext() = default;
    ~RenderContext() = default;

    IRenderTarget<Device>* target = nullptr;
    Viewport viewport = { 0,0,800,600 };
    Mtx4 view = Mtx4::Identity();
    Mtx4 projection = Mtx4::Identity();
    Color clearColor = { 0,0,0,255 };
    //typename Device::MeshHandle mesh = { 0 };
    //typename Device::MaterialHandle material = { 0 };
};