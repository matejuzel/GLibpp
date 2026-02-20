#include "App.h"
#include <iostream>
#include "geometry/Mesh.h"
#include "math/Mtx4.h"
#include "core/input/KeyCode.h"

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

void App::onKeydown(KeyCode keyCode) {

    if (keyCode == KeyCode::Up) {

        mtx = Mtx4::translation(0,0,0.01) * mtx;
    }
    
    if (keyCode == KeyCode::Down) {

        mtx = Mtx4::translation(0, 0, -0.01) * mtx;
    }

    if (keyCode == KeyCode::Left) {

        mtx = Mtx4::rotationY(0.01) * mtx;
    }

    if (keyCode == KeyCode::Right) {

        mtx = Mtx4::rotationY(-0.01) * mtx;
    }

    cout << mtx.toString() << endl;
    //cout << "key code: " << toString(keyCode) << endl;
}
