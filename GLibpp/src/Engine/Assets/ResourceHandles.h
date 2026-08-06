#pragma once

#include "StableRegistry.h"
#include "Mesh.h"

// dopredna deklarace - vnoreny Handle na typu T nezavisi
namespace GLibpp::Geometry { struct MeshInstance; }

namespace GLibpp::Assets {

// jednotny handle system: typ handle = vnoreny typ StableRegistry<T>
// (typova bezpecnost zdarma - MeshHandle nelze priradit do TargetHandle apod.)
using MeshHandle         = Core::StableRegistry<Geometry::Mesh>::Handle;
using MeshInstanceHandle = Core::StableRegistry<Geometry::MeshInstance>::Handle;

inline constexpr MeshHandle         MESH_HANDLE_INVALID          = Core::StableRegistry<Geometry::Mesh>::INVALID;
inline constexpr MeshInstanceHandle MESH_INSTANCE_HANDLE_INVALID = Core::StableRegistry<Geometry::MeshInstance>::INVALID;

}
