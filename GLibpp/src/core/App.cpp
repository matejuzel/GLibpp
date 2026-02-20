#include "App.h"
#include <iostream>
#include "geometry/Mesh.h"
#include "math/Mtx4.h"

using namespace std;

void App::work()
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

void App::onKeydown(int keyCode) {

    cout << "key code: " << keyCode << endl;
}
