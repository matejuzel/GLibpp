#include "App.h"
#include <iostream>
#include <algorithm>
#include "geometry/Mesh.h"
#include "math/Mtx4.h"
#include <Keymap.h>
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowBuilder.h"
#include "utils/timer/Fps.h"
#include "core/Types.h"

using namespace std;

void App::init()
{

    // iniciace Scene State
    sceneState.viewport = {
        0,0,
        this->renderer->window->getWidth(),
        this->renderer->window->getHeight()
    };

    sceneState.projection = Mtx4::perspective(
        45.0f * 3.14159f / 180.0f,
        sceneState.width / (float)sceneState.height,
        0.01f,
        1000.0f
    );

    sceneState.view = Mtx4::lookAt(
        Vec4(10.0f, 1.0f, 3.0f, 1.0f),
        Vec4(0.0f, 0.0f, 0.0f, 1.0f),
        Vec4(0.0f, 1.0f, 0.0f, 0.0f)
    );

    sceneState.meshesStatic = {
        Mesh().addNet(20).transform(Mtx4::scaling(5.0f, 5.0f, 5.0f)).applyTransformation(),
        Mesh().addCube(1.0f).transform(Mtx4::rotationY(0.9f)).applyTransformation(),
        Mesh().addCube(0.3f).transform(Mtx4::translation(2.0f, 1.0f, 0.6f)).applyTransformation(),
    };

    sceneState.meshesDynamic = {
        Mesh().addCube(0.5f),
    };

    sceneState.velocityMove = 0.0f;


    // nakrmeni Render Command Bufferu
    auto& rcb = renderer->getRenderCommandBufferRef();
    
    {
        auto& wb = rcb.writeBuffer();
        wb.clear();
        wb.pushClear(0, 0, 0);
        
        uint32_t mshIdTmp = 0;
        for (const auto& msh : sceneState.meshesStatic) {
            wb.pushDrawMesh(mshIdTmp++, msh.transformation);
        }

    }
    rcb.publish();

    // nakrmeni Render Command Queue
    {
        auto& cmdQ = renderer->getRenderCommandQueue();

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

}

bool App::runGameLoop() {

    //return this->gameLoop.mainLoopBasic();
    //return this->gameLoop.mainLoopFixedTimestamp();
    return this->gameLoop.mainLoopFixedTimestepBufferedAndQueue();
}

void App::update(float dt)
{

    float factorMove = 10.0;
    float factorRotate = 4.0;

    if (keyboard[KEY_UP]) this->sceneState.velocityMove += factorMove * dt;
    if (keyboard[KEY_DOWN]) this->sceneState.velocityMove -= factorMove * dt;
    if (keyboard[KEY_LEFT]) this->sceneState.view = this->sceneState.view * Mtx4::rotationY(-factorRotate * dt);
    if (keyboard[KEY_RIGHT]) this->sceneState.view = this->sceneState.view * Mtx4::rotationY(factorRotate * dt);
    if (keyboard[KEY_SPACE]) this->antialiasing = false;
    if (keyboard[KEY_ENTER]) this->antialiasing = true;

    sceneState.view = sceneState.view * Mtx4::translation(0.0f, 0.0f, dt * sceneState.velocityMove);

    if (abs(sceneState.velocityMove) > 0.01) {

        sceneState.velocityMove -= sceneState.velocityMove * 0.01f;
    }
    else {
        sceneState.velocityMove = 0.0f;
    }

    auto& cmdQ = renderer->getRenderCommandQueue();
    {
        RenderCommand::Command cmd;
        cmd.type = RenderCommand::CommandType::SetMatrixModelview;
        cmd.setMatrixModelView.matrix = sceneState.view;
        cmdQ.push(cmd);
    }
    
    
    //std::this_thread::sleep_for(std::chrono::milliseconds(1 + (rand() % 20)));


	fpsLogic.tick();
}


void App::__cmdUpdate(float dt)
{
    console.clearBack();

    int line = 0;
    console.write(line++, "Matrix:");
    console.write(line++, this->sceneState.view.toString());

    line += 5;

    console.write(line++, "Stisknuto: " + this->keyboard.toString());
    console.write(line++, "velocity move: " + std::to_string(this->sceneState.velocityMove));
    console.write(line++, "Renderer FPS        : " + std::to_string(fps.getFps()));
    console.write(line++, "Renderer FPS (1%)   : " + std::to_string(fps.getLow1Percent()));
    console.write(line++, "Renderer FPS (0.1%): " + std::to_string(fps.getLowPoint1Percent()));
    line++;
    console.write(line++, "Logic FPS           : " + std::to_string(fpsLogic.getFps()));
    console.write(line++, "Logic FPS (1%)      : " + std::to_string(fpsLogic.getLow1Percent()));
    console.write(line++, "Logic FPS (0.1%)    : " + std::to_string(fpsLogic.getLowPoint1Percent()));

    console.present();
}


