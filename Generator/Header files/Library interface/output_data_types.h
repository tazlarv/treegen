/// Output data types of TreeGen library.
/// \file output_data_types.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef OUTPUT_DATA_TYPES_H
#define OUTPUT_DATA_TYPES_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <memory>
#include <type_traits>

#ifndef TREEGEN_AS_DLL
#define TREEGEN_API
#else

#ifdef TREEGEN_EXPORTS
#define TREEGEN_API __declspec(dllexport)
#else
#define TREEGEN_API __declspec(dllimport)
#endif

// Disables warnings from exporting of members without dll-interface
#pragma warning(push)
#pragma warning(disable: 4251) 

#endif

namespace Treegen
{
	/// Vertex of mesh.
	struct TREEGEN_API MeshVertex {
		glm::vec3 position{0.f};
		glm::vec2 textureCoordinates{0.f};
		glm::vec3 normal{0.f};
		glm::vec3 tangent{0.f};

		unsigned Idx = 0;

		MeshVertex() = default;
		explicit MeshVertex(const glm::vec3& position) : position{position} {}
	};

	using rwMeshVertex = std::reference_wrapper<MeshVertex>;
	using rwConstMeshVertex = std::reference_wrapper<const MeshVertex>;

	/// Triangle of mesh.
	struct TREEGEN_API MeshTriangle {
		std::array<rwMeshVertex, 3> vertices;	///< Vertices in counter clockwise order.

		/// Constructs new MeshTriangle object.
		/// Vertices should be in counter clockwise order (\p a \p b \p c).
		MeshTriangle(const rwMeshVertex a, const rwMeshVertex b, const rwMeshVertex c) :
			vertices{a, b, c} {}
	};

	/// Mesh of tree.
	struct TREEGEN_API TreeMesh {
		unsigned treeID;
		std::vector<std::unique_ptr<MeshVertex>> vertices{};
		std::vector<MeshTriangle> triangles{};

		explicit TreeMesh(const unsigned treeID = 0) : treeID{treeID} {}
		TreeMesh(const TreeMesh&) = delete;
		TreeMesh& operator=(const TreeMesh&) = delete;
		TreeMesh(TreeMesh&&) = default;
		TreeMesh& operator=(TreeMesh&&) = default;
		~TreeMesh() = default;
	};

	/// Structure of tree.
	struct TREEGEN_API TreeStructure {
		unsigned treeID;
		std::vector<glm::vec3> nodesPositions{};
		std::vector<glm::vec3> internodes{};

		explicit TreeStructure(const unsigned treeID = 0) : treeID{treeID} {}
	};
}

#ifdef TREEGEN_DLL_EXPORT
#pragma warning(pop)
#endif

#endif // OUTPUT_DATA_TYPES_H
