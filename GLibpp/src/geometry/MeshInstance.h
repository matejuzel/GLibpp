#pragma once

#include <vector>
#include <cstdint>
#include "math/Mtx4.h"
#include "geometry/Mesh.h"

struct MeshInstanceHandle {
    uint32_t index;
    uint32_t generation;
};

static constexpr MeshInstanceHandle INVALID_MESH_INSTANCE = { UINT32_MAX, UINT32_MAX };

class MeshInstance {
public:
    MeshInstance() = default;

    MeshInstance(const MeshInstance&) = delete;
    MeshInstance& operator=(const MeshInstance&) = delete;

    MeshInstance(MeshInstance&&) = default;
    MeshInstance& operator=(MeshInstance&&) = default;

    MeshInstance(mesh_handle_t mesh, material_handle_t material, const Mtx4& transform)
        : mesh(mesh), material(material), transformMatrix(transform), parent(INVALID_MESH_INSTANCE) {
    }

    // --- Gettery ---
    mesh_handle_t getMeshHandle() const { return mesh; }
    material_handle_t getMaterialHandle() const { return material; }

    const Mtx4& getTransform() const { return transformMatrix; }
    void applyTransform(const Mtx4& mtx) { transformMatrix *= mtx; }

    MeshInstanceHandle getParent() const { return parent; }

    const std::vector<MeshInstanceHandle>& getChildren() const { return children; }
    std::vector<MeshInstanceHandle>& getChildren() { return children; }

    // --- Parent / Child API ---
    void setParent(const MeshInstanceHandle& p) { parent = p; }

    void addChild(const MeshInstanceHandle& child) {
        children.push_back(child);
    }

private:
    mesh_handle_t mesh = 0;
    material_handle_t material = 0;
    Mtx4 transformMatrix;

    MeshInstanceHandle parent;
    std::vector<MeshInstanceHandle> children;
};
