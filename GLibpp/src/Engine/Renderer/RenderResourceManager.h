#pragma once

#include "SlotArray.h"

template <typename Device>
struct RenderResourceManager {
public:
    RenderResourceManager() = default;
    //RenderTargetRegistry renderTargetRegistry; // @todo dodelat!!!


    int a = 12;
    //RenderTargetRegistry


    std::unique_ptr<typename Device::Target> target;

    SlotArray<typename Device::Target> targets;





};