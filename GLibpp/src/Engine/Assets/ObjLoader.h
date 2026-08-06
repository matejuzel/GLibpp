#pragma once

#include "IModelLoader.h"
#include <string>

namespace GLibpp::Assets {

	// Loader Wavefront .obj souboru.
	//
	// Podporovano:
	//   v  - pozice vrcholu (x y z; pripadny 4. parametr w a barevne rozsireni se ignoruji)
	//   f  - facy: trojuhelniky i polygony (fan triangulace), 1-based i zaporne
	//        relativni indexy, vsechny tvary "v", "v/vt", "v//vn", "v/vt/vn"
	//
	// Preskakovano (Mesh zatim nese jen pozice + indexy): vt, vn, vp, s, o, g, l, p.
	// Materialy (mtllib/usemtl) se zatim ignoruji - dvirka jsou otevrena v parsovaci
	// smycce: az ModelData dostane materialy, sesbiraji se tam jmeno .mtl a usemtl sub-range.
	//
	// Chyby (soubor nejde otevrit, poskozeny format, zadna geometrie) hlasi
	// std::runtime_error - loading je urceny pro setup fazi (pred resources.freeze()).
	class ObjLoader final : public IModelLoader {
	public:

		bool supportsExtension(std::string_view ext) const override;

		// nacte a naparsuje .obj soubor z disku
		ModelData load(const std::string& path) const override;

		// parsovani primo z textu (testy, generovana / embedded data)
		static Mesh LoadFromString(const std::string& text);
	};

}
