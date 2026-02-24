#include "App.h"
#include <iostream>
#include "geometry/Mesh.h"
#include "math/Mtx4.h"
#include <Keymap.h>

using namespace std;

void App::init()
{
    Vec4 eye(1.0f, 0.0f, -3.0f, 0.0f);
    Vec4 target(0.0f, 0.0f, 0.0f, 0.0f);
    Vec4 up(0.0f, 1.0f, 0.0f, 0.0f);

    mtx = Mtx4::lookAt(eye, target, up);

}

void App::update()
{
    float stepMove = 0.01f;
    float stepRotate = 0.01f;

    if (keyboard[KEY_UP]) mtx = mtx * Mtx4::translation(0.0f, 0.0f, stepMove);
    if (keyboard[KEY_DOWN]) mtx = mtx * Mtx4::translation(0.0f, 0.0f, -stepMove);
    if (keyboard[KEY_LEFT]) mtx = mtx * Mtx4::rotationY(stepRotate);
    if (keyboard[KEY_RIGHT]) mtx = mtx * Mtx4::rotationY(-stepRotate);
}

void App::render()
{
    std::cout << std::string(100, '\n');
    std::cout << mtx.toString() << std::endl;
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
