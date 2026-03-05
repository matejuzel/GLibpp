#include "App.h"
#include <iostream>
#include <algorithm>
#include "geometry/Mesh.h"
#include "math/Mtx4.h"
#include <Keymap.h>
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowBuilder.h"
#include "utils/timer/Fps.h"

using namespace std;

void App::init(int width, int height)
{
    auto& rcb = renderCtx->renderer->getRenderCommandBufferRef();
    
    {
        auto& wb = rcb.writeBuffer();
        wb.clear();

        wb.pushClear();

        
        uint32_t mshIdTmp = 0;
        for (const auto& msh : sceneState.meshesStatic) {
            wb.pushDrawMesh(mshIdTmp++);
        }

    }
    rcb.publish();

}

bool App::runGameLoop() {
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

    auto& cmdQ = renderCtx->renderer->getRenderCommandQueue();
    {
        RenderCommand::Command cmd;
        cmd.type = RenderCommand::CommandType::SetMatrixModelview;
        cmd.setMatrixModelView.matrix = sceneState.view;
        cmdQ.push(cmd);
    }
    
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


