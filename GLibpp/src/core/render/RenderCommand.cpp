#include "RenderCommand.h"

MeshRegistry g_meshRegistry;

namespace RenderCommand {

    void execSetClearColor(const Command& cmd) {
        auto& c = cmd.setClearColor;
        std::cout << "Set clear color to: " << c.r << ", " << c.g << ", " << c.b << std::endl;
    }

    void execClear(const Command& cmd) {
        std::cout << "Clear screen\n";
    }

    void execRegisterMesh(const Command& cmd) {
        g_meshRegistry.registerMesh(cmd.registerMesh.mesh, cmd.registerMesh.meshId);
        std::cout << "Registered mesh with ID: " << cmd.registerMesh.meshId << std::endl;
    }

    void execDrawMesh(const Command& cmd) {
        Mesh* mesh = g_meshRegistry.get(cmd.drawMesh.meshId);
        if (!mesh) {
            std::cout << "Mesh ID not found: " << cmd.drawMesh.meshId << std::endl;
            return;
        }
		std::cout << "Drawing mesh with ID: " << cmd.drawMesh.meshId << std::endl;
        // draw mesh...
    }

    Function dispatchTable[] = {
        &execSetClearColor,
        &execClear,
        &execRegisterMesh,
        &execDrawMesh,
    };

    void exec(const Command& command) {
        dispatchTable[(uintIndex_t)command.type](command);
    }

}


