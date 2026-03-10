
/*
#include "RenderCommand.h"
#include "math/Mtx4.h"
#include <WindowBuilder.h>
#include <App.h>

//MeshRegistry g_meshRegistry;

namespace RenderCommand {

    void execClear(const Command& cmd, Renderer& renderer) 
    {
		renderer.window->DIB_clear(0x00000000);
    }

    void execRegisterMesh(const Command& cmd, Renderer& renderer)
    {
        auto id = cmd.registerMesh.meshId;
        if (id >= renderer.meshRegistry.size())
            renderer.meshRegistry.resize(id + 1, nullptr);

        renderer.meshRegistry[id] = cmd.registerMesh.mesh;
    }


    void execDrawMesh(const Command& cmd, Renderer& renderer) 
    {
		renderer.drawMesh(cmd.drawMesh.meshId, cmd.drawMesh.transformation);
    }

    void execSetMatrixProjection(const Command& cmd, Renderer& renderer)
    {
        renderer.renderContext.projection = cmd.setMatrixProjection.matrix;
    }

    void execSetMatrixModelview(const Command& cmd, Renderer& renderer)
    {
        renderer.renderContext.modelview = cmd.setMatrixModelView.matrix;
    }

    void execSetViewport(const Command& cmd, Renderer& renderer)
    {
        renderer.renderContext.viewport.offsetX = cmd.setViewport.offsetX;
        renderer.renderContext.viewport.offsetY = cmd.setViewport.offsetY;
        renderer.renderContext.viewport.width = cmd.setViewport.width;
        renderer.renderContext.viewport.height = cmd.setViewport.height;
    }

    Function dispatchTable[] = {
        //&execSetClearColor,
        &execClear,
        &execRegisterMesh,
        &execDrawMesh,
        &execSetMatrixProjection,
        &execSetMatrixModelview,
        &execSetViewport,
    };

    void exec(const Command& command, Renderer& renderer) {
        dispatchTable[(uintIndex_t)command.type](command, renderer);
    }

}


*/