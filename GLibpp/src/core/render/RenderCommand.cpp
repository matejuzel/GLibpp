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

    void execSetMatrixProjection(const Command& cmd)
    {
        const Mtx4& mtx = cmd.setMatrixProjection.matrix;
        std::cout << "Set matrix Projection" << std::endl;
        std::cout << mtx.toString() << std::endl;
    }

    void execSetMatrixModelview(const Command& cmd)
    {
        const Mtx4& mtx = cmd.setMatrixModelView.matrix;
        std::cout << "Set matrix ModelView" << std::endl;
        std::cout << mtx.toString() << std::endl;
    }

    void execSetViewport(const Command& cmd)
    {
        uint32_t offsetX = cmd.setViewport.offsetX;
        uint32_t offsetY = cmd.setViewport.offsetY;
        uint32_t width = cmd.setViewport.width;
        uint32_t height = cmd.setViewport.height;
        std::cout << "Set viewport(" << offsetX << "," << offsetY << "," << width << "," << height << ")" << std::endl;
    }

    Function dispatchTable[] = {
        &execSetClearColor,
        &execClear,
        &execRegisterMesh,
        &execDrawMesh,
        &execSetMatrixProjection,
        &execSetMatrixModelview,
        &execSetViewport,
    };

    void exec(const Command& command) {
        dispatchTable[(uintIndex_t)command.type](command);
    }

}


