/// Supporting data structures for usage of TreeGen library.
/// \file treegen_management.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef TREEGEN_MANAGEMENT_H
#define TREEGEN_MANAGEMENT_H

#include <treegen_library.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <type_traits>


/// Representation of tree generator library and trees inside of it.
struct TreegenLib {
	std::unique_ptr<Treegen::Generator> generator = Treegen::Generator::createGenerator();
	std::vector<std::reference_wrapper<Treegen::Tree>> trees{};
};


/// All tree data (tree structure and mesh).
struct TreesData {
	std::vector<std::vector<std::unique_ptr<Treegen::MeshVertex>>> vertices{};
	std::vector<std::vector<Treegen::MeshTriangle>> triangles{};

	std::vector<std::vector<glm::vec3>> nodesPositions{};
	std::vector<std::vector<glm::vec3>> internodes{};

	size_t verticesCount = 0;
	size_t trianglesCount = 0;

	size_t nodesPositionsCount = 0;
	size_t internodesCount = 0;

	/// Clears containers and resets counters.
	void clear() {
		vertices.clear();
		triangles.clear();
		nodesPositions.clear();
		internodes.clear();

		verticesCount = 0;
		trianglesCount = 0;
		nodesPositionsCount = 0;
		internodesCount = 0;
	}
};


#endif // TREEGEN_MANAGEMENT_H
