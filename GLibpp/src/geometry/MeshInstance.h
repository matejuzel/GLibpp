#pragma once

#include <vector>
#include <cstdint>
#include "math/Mtx4.h"
#include "geometry/Mesh.h"

using entity_id = uint32_t;
static constexpr entity_id INVALID_ENTITY = UINT32_MAX;

class MeshInstance {
public:
    MeshInstance() = default;

    MeshInstance(mesh_handle_t mesh, material_handle_t material, const Mtx4& transform)
        : mesh(mesh), material(material), transformMatrix(transform), parent(INVALID_ENTITY) {}

    MeshInstance(mesh_handle_t mesh, material_handle_t material) 
        : mesh(mesh), material(material), transformMatrix(Mtx4::identity()), parent(INVALID_ENTITY) {}

    mesh_handle_t getMeshHandle() const { return mesh; }
    material_handle_t getMaterialHandle() const { return material; }

    const Mtx4& getTransform() const { return transformMatrix; }
    void applyTransform(const Mtx4& mtx) { transformMatrix *= mtx; }

    entity_id getParent() const { return parent; }
    const std::vector<entity_id>& getChildren() const { return children; }
    std::vector<entity_id>& getChildren() { return children; }

    void setParent(entity_id p) { parent = p; }
    void addChild(entity_id childId) { children.push_back(childId); }

private:
    mesh_handle_t mesh = 0;
    material_handle_t material = 0;
    Mtx4 transformMatrix;

    entity_id parent = INVALID_ENTITY;
    std::vector<entity_id> children;
};
