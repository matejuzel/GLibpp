#pragma once

#include <vector>
#include "utils/datastruct/TripleBuffer.h"
#include "geometry/Mesh.h"

typedef std::vector<Mesh*> registry_t;
typedef TripleBuffer<registry_t> tbuff_registry_t;
typedef uint32_t msh_handler_t;

class MeshRegistry {
public:

	MeshRegistry() = default;

	msh_handler_t registerMesh(Mesh& msh) 
	{
		registry.push_back(&msh);
		return static_cast<msh_handler_t>(registry.size() - 1);
	}

	Mesh* get(msh_handler_t handler) 
	{
		return registry[handler];
	}

	void publish() {
		
		registry_t& wb = snapshot.writeBuffer();
		wb.clear();
		wb = registry;
		snapshot.publish();
	}

	const registry_t& getSnapshot() {

		const registry_t& rb = snapshot.readBuffer();
		return rb;
	}


private:

	registry_t registry;
	tbuff_registry_t snapshot;
};