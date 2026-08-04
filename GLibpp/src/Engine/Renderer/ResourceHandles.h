#pragma once

#include "StableRegistry.h"
#include "Mesh.h"

// dopredna deklarace - vnoreny Handle na typu T nezavisi
struct MeshInstance;

// jednotny handle system: typ handle = vnoreny typ StableRegistry<T>
// (typova bezpecnost zdarma - MeshHandle nelze priradit do TargetHandle apod.)
using MeshHandle         = StableRegistry<Mesh>::Handle;
using MeshInstanceHandle = StableRegistry<MeshInstance>::Handle;

inline constexpr MeshHandle         MESH_HANDLE_INVALID   = StableRegistry<Mesh>::INVALID;
inline constexpr MeshInstanceHandle MESH_INSTANCE_INVALID = StableRegistry<MeshInstance>::INVALID;
