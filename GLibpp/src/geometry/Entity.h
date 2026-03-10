#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include "math/Mtx4.h"
#include "geometry/Mesh.h"


class Entity {
public:

	using entity_unique_ptr = std::unique_ptr<Entity>;
	using entity_list_t     = std::vector<entity_unique_ptr>;

	static entity_unique_ptr Make(mesh_handle_t meshHandle, material_handle_t materialHandle, Mtx4 transform);
	Entity(mesh_handle_t mesh, material_handle_t material, Mtx4 transformation) : mesh(mesh), material(material), transformation(transformation) {};
	Entity(const Entity&) = delete;
	Entity& operator=(const Entity&) = delete;
	Entity (Entity&&) = default;
	Entity& operator=(Entity&&) = default;
	Entity& addChild(entity_unique_ptr entity);
	mesh_handle_t getMeshHandle() const;
	material_handle_t getMaterialHandle() const;
	void transform(const Mtx4& mtx);
	const Mtx4& getTransformation() const;
	const entity_list_t& getChildren() const;
	entity_list_t& getChildren();
	Entity* getParent();
	const Entity* getParent() const;

private:

	mesh_handle_t mesh;
	material_handle_t material;
	Mtx4 transformation;

	entity_list_t children;
	Entity* parent = nullptr;
};