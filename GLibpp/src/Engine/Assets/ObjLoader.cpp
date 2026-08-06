#include "ObjLoader.h"

#include <charconv>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

	bool isSpace(char c) {
		return c == ' ' || c == '\t';
	}

	// orizne uvodni/koncove mezery a pripadny \r (soubory s CRLF ctene po \n)
	std::string_view trim(std::string_view s) {
		while (!s.empty() && isSpace(s.front())) s.remove_prefix(1);
		while (!s.empty() && (isSpace(s.back()) || s.back() == '\r')) s.remove_suffix(1);
		return s;
	}

	// vrati dalsi whitespace-oddeleny token a posune vstup za nej; prazdny token = konec radku
	std::string_view nextToken(std::string_view& s) {
		while (!s.empty() && isSpace(s.front())) s.remove_prefix(1);
		size_t end = 0;
		while (end < s.size() && !isSpace(s[end])) ++end;
		std::string_view token = s.substr(0, end);
		s.remove_prefix(end);
		return token;
	}

	[[noreturn]] void fail(const std::string& what, size_t lineNo) {
		throw std::runtime_error("ObjLoader: " + what + " (radek " + std::to_string(lineNo) + ")");
	}

	// parsovani floatu pres from_chars - nezavisle na locale (ceska Windows!)
	float parseFloat(std::string_view token, size_t lineNo) {
		if (token.empty()) fail("chybi ciselna hodnota", lineNo);
		if (token.front() == '+') token.remove_prefix(1); // from_chars '+' neakceptuje
		float value = 0.0f;
		auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), value);
		if (ec != std::errc()) fail("neplatne cislo '" + std::string(token) + "'", lineNo);
		return value;
	}

	// z tokenu tvaru "v", "v/vt", "v//vn" nebo "v/vt/vn" vytahne index pozice
	// a prevede ho na 0-based (1-based i zaporne relativni indexy dle .obj specifikace);
	// vt/vn indexy se zatim zahazuji - Mesh nese jen pozice (dvirka pro rozsireni)
	uint32_t parsePositionIndex(std::string_view token, size_t vertexCount, size_t lineNo) {
		size_t slash = token.find('/');
		std::string_view indexPart = (slash == std::string_view::npos) ? token : token.substr(0, slash);

		long long index = 0;
		auto [ptr, ec] = std::from_chars(indexPart.data(), indexPart.data() + indexPart.size(), index);
		if (ec != std::errc() || index == 0) fail("neplatny index facu '" + std::string(token) + "'", lineNo);

		// zaporny index = relativne od konce dosud nactenych vrcholu (-1 = posledni)
		long long resolved = (index > 0) ? index - 1 : static_cast<long long>(vertexCount) + index;
		if (resolved < 0 || resolved >= static_cast<long long>(vertexCount)) {
			fail("index facu '" + std::string(token) + "' miri mimo dosud nactene vrcholy", lineNo);
		}
		return static_cast<uint32_t>(resolved);
	}

}

namespace GLibpp::Assets {

	bool ObjLoader::supportsExtension(std::string_view ext) const
	{
		return ext == ".obj";
	}

	Mesh ObjLoader::LoadFromString(const std::string& text)
	{
		std::vector<Vec4> positions;
		std::vector<uint32_t> indices;

		std::string_view rest(text);
		size_t lineNo = 0;
		std::vector<uint32_t> faceIndices; // scratch pro jeden fac, alokuje se jen jednou

		while (!rest.empty()) {
			size_t newline = rest.find('\n');
			std::string_view line = rest.substr(0, newline == std::string_view::npos ? rest.size() : newline);
			rest.remove_prefix(newline == std::string_view::npos ? rest.size() : newline + 1);
			++lineNo;

			line = trim(line);
			if (line.empty() || line.front() == '#') continue;

			std::string_view keyword = nextToken(line);

			if (keyword == "v") {
				float x = parseFloat(nextToken(line), lineNo);
				float y = parseFloat(nextToken(line), lineNo);
				float z = parseFloat(nextToken(line), lineNo);
				// pripadny 4. parametr (w) a barevne rozsireni (r g b) ignorujeme - w drzime 1
				positions.emplace_back(x, y, z, 1.0f);
			}
			else if (keyword == "f") {
				faceIndices.clear();
				for (std::string_view token = nextToken(line); !token.empty(); token = nextToken(line)) {
					faceIndices.push_back(parsePositionIndex(token, positions.size(), lineNo));
				}
				if (faceIndices.size() < 3) fail("fac ma mene nez 3 vrcholy", lineNo);

				// fan triangulace - trojuhelniky projdou beze zmeny, polygony se rozpadnou
				for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
					indices.push_back(faceIndices[0]);
					indices.push_back(faceIndices[i]);
					indices.push_back(faceIndices[i + 1]);
				}
			}
			else if (keyword == "mtllib" || keyword == "usemtl") {
				// materialy zatim nepodporujeme; az ModelData dostane materialy,
				// tady se sesbira jmeno .mtl souboru a hranice usemtl skupin
			}
			// vt, vn, vp, s, o, g, l, p a nezname keywordy tise preskakujeme
		}

		if (positions.empty() || indices.empty()) {
			throw std::runtime_error("ObjLoader: data neobsahuji zadnou pouzitelnou geometrii (v + f)");
		}

		return MeshAccess::Create(std::move(positions), std::move(indices));
	}

	ModelData ObjLoader::load(const std::string& path) const
	{
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			throw std::runtime_error("ObjLoader: nelze otevrit soubor: " + path);
		}

		std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		try {
			return ModelData{ LoadFromString(text) };
		}
		catch (const std::exception& e) {
			// pridame cestu, aby chybova hlaska rekla i KTERY soubor je poskozeny
			throw std::runtime_error(std::string(e.what()) + " [" + path + "]");
		}
	}

}
