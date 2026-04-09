#pragma once
#include <cstdint>

struct DeviceTargetHandle {
	uint32_t handle; // opaque identifier for the render target (e.g., backbuffer index)
};