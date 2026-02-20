// GLibpp.cpp : Tento soubor obsahuje funkci main. Provádění programu se tam zahajuje a ukončuje.
//

#include <iostream>

#include "Mtx4.h"

#include "Mesh.h"

using namespace std;

int main()
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
    );

    msh.applyTransformation();

    cout << msh.computeAABB().toString() << endl;

}

