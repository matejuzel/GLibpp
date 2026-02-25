#include "App.h"
#include <iostream>
#include "geometry/Mesh.h"
#include "math/Mtx4.h"
#include "window/WindowBuilder.h"
#include <Keymap.h>
#include <windows.h>
#include "window/ConsoleDoubleBuffer.h"

using namespace std;

void App::init()
{
    Vec4 eye(1.0f, 0.0f, 0.0f, 0.0f);
    Vec4 target(0.0f, 0.0f, 0.0f, 0.0f);
    Vec4 up(0.0f, 1.0f, 0.0f, 0.0f);

    mtx = Mtx4::lookAt(eye, target, up);

}

void App::update(float dt)
{
    float factorMove = 8.0;
    float factorRotate = 1.0;

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
    
}


void App::__cmdUpdate(float dt)
{

    console.clearBack();
    console.write(0, "Matrix:");
    console.write(1, this->sceneState.transformation.toString());

    console.write(6, "Stisknuto: " + this->keyboard.toString());
    console.write(7, "velocity move: " + std::to_string(this->sceneState.velocityMove));
    //console.write(8, "velocity move: " + std::to_string(this->sceneState.velocityRotation));

    console.present();

    //WindowBuilder::consolePrint(mtx.toString());
}

void App::__work()
{
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
}

