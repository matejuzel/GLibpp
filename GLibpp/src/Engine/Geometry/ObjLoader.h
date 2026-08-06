#pragma once

#include "Mesh.h"
#include <string>

// Nacitani geometrie z Wavefront .obj souboru.
//
// Podporovano:
//   v  - pozice vrcholu (x y z; pripadny 4. parametr w a barevne rozsireni se ignoruji)
//   f  - facy: trojuhelniky i polygony (fan triangulace), 1-based i zaporne
//        relativni indexy, vsechny tvary "v", "v/vt", "v//vn", "v/vt/vn"
//
// Preskakovano (Mesh zatim nese jen pozice + indexy): vt, vn, vp, s, o, g, l, p.
// Materialy (mtllib/usemtl) se zatim ignoruji - dvirka jsou otevrena v parsovaci
// smycce: az Mesh dostane materialy, sesbiraji se tam jmeno .mtl a usemtl sub-range.
//
// Chyby (soubor nejde otevrit, poskozeny format, zadna geometrie) hlasi
// std::runtime_error - loader je urceny pro setup fazi (pred resources.freeze()).
class ObjLoader {
public:

	// Nacte a naparsuje .obj soubor z disku
	static Mesh LoadFromFile(const std::string& path);

	// Parsovani primo z textu (testy, generovana / embedded data)
	static Mesh LoadFromString(const std::string& text);
};
