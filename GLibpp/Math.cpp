// Math.cpp : Tento soubor obsahuje funkci main. Provádění programu se tam zahajuje a ukončuje.
//

#include <iostream>
#include <string>
#include "Mtx4.h"


using namespace std;



int main()
{
    Mtx4 m1 = Mtx4(
        1,2,3,4,
        2,2,3,5,
        3,1,1,2,
        5,1,2,4
    );

    Mtx4 m2 = Mtx4(
        2,2,4,5,
        2,2,4,6,
        1,1,5,2,
        1,6,3,2
    );

    cout << m1.toString() << endl;
    cout << m2.toString() << endl;

    cout << (m1 * m2).toString() << endl;
    //cout << (m1 *= m2).toString() << endl;
    cout << (m1.multiply(m2)).toString() << endl;


    
    
    




}
