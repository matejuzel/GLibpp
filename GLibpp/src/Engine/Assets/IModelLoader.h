#pragma once

#include "Mesh.h"
#include <string>
#include <string_view>
#include <vector>

namespace GLibpp::Assets {

	// vysledek nacteni modelu - dnes jen geometrie; pozdeji sem pribudou
	// materialy (.mtl), submeshe/usemtl range apod.
	struct ModelData {
		Geometry::Mesh mesh;
	};

	// jediny friend Meshe za vsechny loadery - novy format uz Mesh.h needituje;
	// loader si naparsuje lokalni buffery a tady je presune do Meshe (zadne kopie)
	struct MeshAccess {
		static Geometry::Mesh Create(std::vector<Vec4>&& positions, std::vector<uint32_t>&& indices)
		{
			Geometry::Mesh msh;
			msh.vertexBuffer = std::move(positions);
			msh.indexBuffer = std::move(indices);
			return msh;
		}
	};

	// spolecne rozhrani loaderu modelu; implementace registruje ModelImporter,
	// ktery podle pripony souboru vybira spravny loader
	// (virtualy jsou tady v poradku - loading bezi jen v setup fazi, ne v hot path)
	class IModelLoader {
	public:
		virtual ~IModelLoader() = default;

		// umi loader danou priponu? (vcetne tecky, lowercase - napr. ".obj")
		virtual bool supportsExtension(std::string_view ext) const = 0;

		// nacte model ze souboru; chyby hlasi std::runtime_error
		virtual ModelData load(const std::string& path) const = 0;
	};

}
