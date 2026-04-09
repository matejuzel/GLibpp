#pragma once

// Závislé typy (pøedpokládá, že include search path je nastaven)
#include "Mtx4.h"
#include "Color.h"
#include "DeviceTargetHandle.h"

// Render command structures...
struct RCMD_SetViewport { 
	uint32_t x; 
	uint32_t y; 
	uint32_t width; 
	uint32_t height; 
};

struct RCMD_SetMatrixProjection { 
	//Mtx4 matrix; 
	const float *matrix; // pro jednoduchost a zaroven zachovani triviality struktury pouzijeme pole floatu namisto Mtx4
};

struct RCMD_SetMatrixView { 
	//Mtx4 matrix; 
	const float *matrix; // pro jednoduchost a zaroven zachovani triviality struktury pouzijeme pole floatu namisto Mtx4
};

struct RCMD_BindMesh { 
	MeshHandle mesh; 
	MaterialHandle material; 
};

struct RCMD_BindTarget { 
	DeviceTargetHandle hwnd; 
};

struct RCMD_Draw { 
	uint32_t dummy; 
};

struct RCMD_Clear { 
	Color color; 
};