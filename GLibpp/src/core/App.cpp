#include "App.h"
#include <iostream>
#include <algorithm>
#include "geometry/Mesh.h"
#include "math/Mtx4.h"
#include <Keymap.h>
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowBuilder.h"

using namespace std;

void App::init()
{
    // Use Mesh helper to compute view matrix that fits mesh into the screen
    const float verticalFovDeg = 45.0f;
    const float verticalFov = verticalFovDeg * 3.14159265358979323846f / 180.0f;

    // derive aspect from viewport matrix (viewport.at(0,0) == width/2, viewport.at(1,1) == height/2)
    float aspect = this->viewport.at(0,0) / this->viewport.at(1,1);

    // compute view using mesh helper, but include scene transform as well
    Mtx4 worldM = this->sceneState.transformation * this->sceneState.mesh.transformation;
    Mesh tmp = this->sceneState.mesh;
    tmp.transformation = worldM;
    Mtx4 view = tmp.computeViewMatrixToFillScreen(verticalFov, aspect);
    // store both for rendering and debug (render uses `lookAt` matrix)
    this->lookAt = view;
    this->mtx = view;

    // default near/far - good enough for initial scene
    float nearZ = 0.01f;
    float farZ = 1000.0f;
    this->projection = Mtx4::perspective(verticalFov, aspect, nearZ, farZ);

}

void App::update(float dt)
{
    float factorMove = 10.0;
    float factorRotate = 4.0;

    if (keyboard[KEY_UP]) this->sceneState.velocityMove += factorMove * dt;
    if (keyboard[KEY_DOWN]) this->sceneState.velocityMove -= factorMove * dt;
    if (keyboard[KEY_LEFT]) this->sceneState.transformation = this->sceneState.transformation * Mtx4::rotationY(factorRotate * dt);
    if (keyboard[KEY_RIGHT]) this->sceneState.transformation = this->sceneState.transformation * Mtx4::rotationY(-factorRotate * dt);


    sceneState.transformation = sceneState.transformation * Mtx4::translation(0.0f, 0.0f, dt * sceneState.velocityMove);

    if (abs(sceneState.velocityMove) > 0.01) {

        sceneState.velocityMove -= sceneState.velocityMove * 0.01;
    }
    else {
        sceneState.velocityMove = 0.0f;
    }
}


void App::render()
{
    
    this->win->DIB_clear(0x00000000);

    Mtx4 mshTransformation = this->sceneState.mesh.transformation;
    Mtx4 worldTransformation = this->sceneState.transformation;
    Mtx4 mvp = this->projection * this->lookAt * worldTransformation * mshTransformation;

    const auto& tris = this->sceneState.mesh.tris;
    for (const auto& triangle : tris) {
        
        Vec4 a_ = mvp * triangle.a.pos;
        Vec4 b_ = mvp * triangle.b.pos;
        Vec4 c_ = mvp * triangle.c.pos;

        a_.divideW();
        b_.divideW();
        c_.divideW();

        a_ = this->viewport * a_;
        b_ = this->viewport * b_;
        c_ = this->viewport * c_;

        if (!this->win) continue;

        // draw triangle edges
        this->win->DIB_drawTriangle(a_, b_, c_, 0xffff0000);
    }
    


    /*
    float x = this->sceneState.transformation.at(0, 3);
    float z = this->sceneState.transformation.at(2, 3);
    this->win->DIB_drawCircle(x+400, z+300, max(4, abs(this->sceneState.velocityMove/10.0f)), 0xffff0000);
    this->win->DIB_drawCircle(x + 400, z + 300, 4, 0xffffff00);
    */
    this->win->DIB_drawBitmap();
}



void App::__cmdUpdate(float dt, float fps)
{

    console.clearBack();
    console.write(0, "Matrix:");
    console.write(1, this->sceneState.transformation.toString());

    console.write(6, "Stisknuto: " + this->keyboard.toString());
    console.write(7, "velocity move: " + std::to_string(this->sceneState.velocityMove));
    console.write(8, "FPS: " + std::to_string(fps));
    //console.write(8, "velocity move: " + std::to_string(this->sceneState.velocityRotation));

    console.present();

    //WindowBuilder::consolePrint(mtx.toString());
}



void App::setWindow(WindowBuilder *window)
{
    this->win = window;
}

void App::__work()
{

    /*
    Mtx4 m;

    m.translate(1, 2, 3);
    m.rotateX(0.3f);
    m.rotateY(0.21f);

    cout << m.toStringDetail() << endl;

    Mesh msh = Mesh();
    msh.addCube(1).transform(
        Mtx4::identity()
        .translate(0.2f, 0.1f, -0.1f)
        .rotateX(0.01f)
        .rotateY(-0.14f)
        .rotateZ(0.02f)
        .translate(0.32f, 0.0f, 0.0f)
        .scale(3.0f, 0.5f, 1.0f)
    );

    msh.applyTransformation();

    cout << msh.computeAABB().toString() << endl;
    */
}

