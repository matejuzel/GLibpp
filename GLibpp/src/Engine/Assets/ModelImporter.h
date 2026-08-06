#pragma once

#include "IModelLoader.h"
#include "ObjLoader.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace GLibpp::Assets {

	// vstupni bod nacitani modelu: drzi registrovane loadery a podle pripony
	// souboru vybira ten spravny; novy format = nova implementace IModelLoader
	// + registerLoader() (nebo pridani mezi vestavene v konstruktoru)
	class ModelImporter {
	public:

		ModelImporter()
		{
			// vestavene loadery
			loaders.push_back(std::make_unique<ObjLoader>());
		}

		void registerLoader(std::unique_ptr<IModelLoader> loader)
		{
			loaders.push_back(std::move(loader));
		}

		ModelData load(const std::string& path) const
		{
			std::string ext = lowercaseExtension(path);
			for (const auto& loader : loaders) {
				if (loader->supportsExtension(ext)) {
					return loader->load(path);
				}
			}
			throw std::runtime_error("ModelImporter: nepodporovany format '" + ext + "': " + path);
		}

	private:

		// pripona od posledni tecky vcetne, ASCII lowercase (napr. ".obj"); bez pripony = ""
		static std::string lowercaseExtension(const std::string& path)
		{
			size_t dot = path.find_last_of('.');
			size_t sep = path.find_last_of("/\\");
			// tecka pred poslednim oddelovacem cesty neni pripona (napr. "data.v2/model")
			if (dot == std::string::npos || (sep != std::string::npos && dot < sep)) return {};

			std::string ext = path.substr(dot);
			for (char& c : ext) {
				if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
			}
			return ext;
		}

		std::vector<std::unique_ptr<IModelLoader>> loaders;
	};

}
