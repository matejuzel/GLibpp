#include "App.h"
#include <iostream>
#include <algorithm>
#include "geometry/Mesh.h"
#include "math/Mtx4.h"
#include <Keymap.h>
#include "window/ConsoleDoubleBuffer.h"
#include "window/WindowBuilder.h"

using namespace std;

void App::init(int width, int height)
{

	float w_half = width / 2.0f;
	float h_half = height / 2.0f;

    float w_4 = width / 4.0f;
    float h_4 = height / 4.0f;

    this->sceneState.viewportPersp = Mtx4(
           w_4,   0.0f,  0.0f,   w_4,
          0.0f,    w_4,  0.0f,   w_4,
          0.0f,   0.0f,  1.0f,  0.0f,
          0.0f,   0.0f,  0.0f,  1.0f
    );

    this->sceneState.viewportTop = Mtx4(
        w_4, 0.0f, 0.0f, w_4 + w_4,
        0.0f, w_4, 0.0f, w_4 + w_4,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    // Use Mesh helper to compute view matrix that fits mesh into the screen
    const float verticalFovDeg = 45.0f;
    const float verticalFov = verticalFovDeg * 3.14159265358979323846f / 180.0f;
    float aspect = this->sceneState.viewportPersp.at(0,0) / this->sceneState.viewportPersp.at(1,1);

    this->sceneState.viewPersp = Mtx4::lookAt(Vec4(10, 0, 0, 1), Vec4(0, 0, 0, 1), Vec4(0, 1, 0, 0));
    this->sceneState.viewTop = Mtx4::lookAt(Vec4(0, 50, 0, 1), Vec4(0, 0, 0, 1), Vec4(0, 0, 1, 0));

    this->sceneState.projectionPersp = Mtx4::perspective(verticalFov, aspect, 0.01f, 1000.0f);
    this->sceneState.projectionTop = Mtx4::perspective(verticalFov, aspect, 0.01f, 1000.0f);
}

bool App::runGameLoop() {
    return this->gameLoop.mainLoopFixedTimestamp();
}

void App::update(float dt)
{
    float factorMove = 10.0;
    float factorRotate = 4.0;

    if (keyboard[KEY_UP]) this->sceneState.velocityMove += factorMove * dt;
    if (keyboard[KEY_DOWN]) this->sceneState.velocityMove -= factorMove * dt;
    if (keyboard[KEY_LEFT]) this->sceneState.transformation = this->sceneState.transformation * Mtx4::rotationY(-factorRotate * dt);
    if (keyboard[KEY_RIGHT]) this->sceneState.transformation = this->sceneState.transformation * Mtx4::rotationY(factorRotate * dt);


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

    Mtx4 mvp = this->sceneState.projectionPersp * this->sceneState.viewPersp * worldTransformation * mshTransformation;
    Mtx4 mvpTop = this->sceneState.projectionTop * this->sceneState.viewTop * worldTransformation * mshTransformation;

    const auto& tris = this->sceneState.mesh.tris;
    for (const auto& triangle : tris) {
        
        Vec4 a_, b_, c_, aP_, bP_, cP_, aT_, bT_, cT_;

        // persp
        a_ = mvp * triangle.a.pos;
        b_ = mvp * triangle.b.pos;
        c_ = mvp * triangle.c.pos;

        a_.divideW();
        b_.divideW();
        c_.divideW();

        aP_ = this->sceneState.viewportPersp * a_;
        bP_ = this->sceneState.viewportPersp * b_;
        cP_ = this->sceneState.viewportPersp * c_;

        // top
        a_ = mvpTop * triangle.a.pos;
        b_ = mvpTop * triangle.b.pos;
        c_ = mvpTop * triangle.c.pos;

        a_.divideW();
        b_.divideW();
        c_.divideW();

        aT_ = this->sceneState.viewportTop * a_;
        bT_ = this->sceneState.viewportTop * b_;
        cT_ = this->sceneState.viewportTop * c_;

        if (!this->win) continue;

		bool antialiasing = true;

        // draw triangle edges
        this->win->DIB_drawTriangle(aP_, bP_, cP_, 0xffff0000, antialiasing);
        this->win->DIB_drawTriangle(aT_, bT_, cT_, 0xffff0000, antialiasing);
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

