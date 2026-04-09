#pragma once

#include <cstdint>
#include "Mtx4.h"
#include "Color.h"
#include "RenderCommandType.h"
#include "RenderCommandData.h"

struct RenderCommand { 
	RenderCommandType type; 
	RenderCommandData data; 
};


