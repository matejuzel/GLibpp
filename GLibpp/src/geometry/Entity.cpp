#include "Entity.h"
#include "geometry/Mesh.h"

Entity::entity_unique_ptr Entity::Make(mesh_handle_t meshHandle, material_handle_t materialHandle, Mtx4 transform) {
	return std::make_unique<Entity>(meshHandle, materialHandle, transform);
}

Entity& Entity::addChild(entity_unique_ptr entity)
{
	entity->parent = this;
	children.push_back(std::move(entity));
	return *children.back();
}

mesh_handle_t Entity::getMeshHandle() const
{
	return mesh;
}

material_handle_t Entity::getMaterialHandle() const
{
	return material;
}

const Mtx4& Entity::getTransformation() const
{
	return transformation;
}

void Entity::transform(const Mtx4& mtx)
{
	transformation *= mtx;
}

const Entity::entity_list_t& Entity::getChildren() const
{
	return children;
}

Entity::entity_list_t& Entity::getChildren()
{
	return children;
}

Entity* Entity::getParent()
{
	return parent;
}

const Entity* Entity::getParent() const {
	return parent;
}
