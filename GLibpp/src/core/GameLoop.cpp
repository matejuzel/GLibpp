#include "core/GameLoop.h"
#include "core/App.h"
#include "utils/timer/HighResTimer.h"
#include "utils/timer/FixedTimestep.h"
#include <RenderCommand.h>
#include <Renderer.h>

bool GameLoop::mainLoopFixedTimestepBufferedAndQueue() 
{

    App& app = App::instance();
    auto& renderer = *app.renderCtx->renderer;
    
    

    {
		auto& sceneState = app.sceneState;
        auto& cmdQ = renderer.getRenderCommandQueue();

        {
            RenderCommand::Command cmd;
            cmd.type = RenderCommand::CommandType::SetMatrixProjection;
            cmd.setMatrixProjection.matrix = sceneState.projection;
            cmdQ.push(cmd);
        }

        {
            RenderCommand::Command cmd;
            cmd.type = RenderCommand::CommandType::SetMatrixModelview;
			cmd.setMatrixModelView.matrix = sceneState.view;
            cmdQ.push(cmd);
        }
        
        {
            RenderCommand::Command cmd;
            cmd.type = RenderCommand::CommandType::SetViewport;
            cmd.setViewport.offsetX = sceneState.viewport.offsetX;
            cmd.setViewport.offsetY = sceneState.viewport.offsetY;
            cmd.setViewport.width = sceneState.viewport.width;
            cmd.setViewport.height = sceneState.viewport.height;
            cmdQ.push(cmd);
        }

        uint32_t idTmp = 0;
        for (const Mesh& msh : sceneState.meshesStatic) {

            {
                RenderCommand::Command cmd;
                cmd.type = RenderCommand::CommandType::RegisterMesh;
				cmd.registerMesh.mesh = const_cast<Mesh*>(&msh);
				cmd.registerMesh.meshId = idTmp++;
                cmdQ.push(cmd);
            }
        }
    }

    std::thread t_renderLoop(&Renderer::runRenderLoop, &renderer);

    MSG msg = {};

    HighResTimer timer;
    FixedTimestep taskLogic(60.0);
    FixedTimestep taskCmd(60.0);
    
    while (renderer.isRunning())
    {
        // messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
				renderer.stop();
                break;
            };
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // èas
        double frameTime = timer.tick();
        if (frameTime > 0.25) frameTime = 0.25;

        int steps;

        steps = taskLogic.consume(frameTime);
        for (int i = 0; i < steps; i++) {
            app.update((float)taskLogic.getDt());
        }

        steps = taskCmd.consume(frameTime);
        for (int i = 0; i < steps; i++) {

            app.__cmdUpdate((float)taskCmd.getDt());
        }
    }

	renderer.stop();
	
    t_renderLoop.join();

    return true;
}

bool GameLoop::mainLoopFixedTimestamp()
{
    App& app = App::instance();
    app.init(1200, 800);

    MSG msg = {};

    HighResTimer timer;
    FixedTimestep taskLogic(60.0);
    FixedTimestep taskCmd(60.0);

    int frames = 0;

    while (true)
    {
        // messages
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // èas
        double frameTime = timer.tick();
        if (frameTime > 0.25) frameTime = 0.25;

        int steps;

        steps = taskLogic.consume(frameTime);
        for (int i = 0; i < steps; i++) {
            app.update((float)taskLogic.getDt());
        }

        steps = taskCmd.consume(frameTime);
        for (int i = 0; i < steps; i++) {

            float fps = (float)(frames / timer.sinceStart());

            app.__cmdUpdate((float)taskCmd.getDt());
        }

        // render ...

        frames++;
    }

    return true;
}

bool GameLoop::mainLoopBasic()
{
    App& app = App::instance();
    app.init(1200, 800);

    MSG msg = {};
    while (true)
    {

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return true;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        app.update(0.01f);
        // render ...

        Sleep(10);
    }

    return true;
}
