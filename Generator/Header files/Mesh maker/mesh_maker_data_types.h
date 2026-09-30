/// Basic classes for representation of tree structure during making of mesh.
/// \file mesh_maker_data_types.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef MESH_MAKER_DATA_TYPES_H
#define MESH_MAKER_DATA_TYPES_H

#include <Mesh maker/cubic_spline_3D.h>
#include <Tree primitives/tree_primitives.h>
#include <Library interface/output_data_types.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace Treegen
{
	namespace MeshMaker
	{
		struct StructureNode;
		struct StructureBranch;
		struct MeshBranch;

		using rwStructNode = std::reference_wrapper<StructureNode>;
		using rwStructBranch = std::reference_wrapper<StructureBranch>;
		using rwMeshBranch = std::reference_wrapper<MeshBranch>;

		using rwConstStructNode = std::reference_wrapper<const StructureNode>;
		using rwConstStructBranch = std::reference_wrapper<const StructureBranch>;
		using rwConstMeshBranch = std::reference_wrapper<const MeshBranch>;

		/// Representation of node of tree structure.
		/// Axes of tree structure are: root, tips of branches and branching nodes.
		struct StructureNode {
			/// Constructs new StructureNode object.
			/// \param represents TreeNode to be represented by this object.
			explicit StructureNode(const TreeNode& represents) : represents{represents} {}

			const TreeNode& represents;	///< Represented node

			/// \name Pointers for traversal of tree structure from this node.
			///@{
			StructureNode* predecessor = nullptr;
			std::vector<rwStructNode> following{};

			StructureNode* precedingAxis = nullptr;
			std::vector<rwStructNode> followingAxes{};
			///@}

			float branchRadius = 0.f;	///< Variable used for calculation of branch radius.

			/// \name Node type checks.
			///@{
			/// Checks if node is root of tree (it does not have predecessor).
			bool isRoot() const;

			/// Checks if node is tip of branch (does not have any followers).
			bool isBranchTip() const;

			/// Checks if node is branching node of tree (have more than one follower).
			bool isBranchingNode() const;

			/// Checks if node has exactly one follower..
			bool isInnerNode() const;
			///@}
		};

		inline bool operator==(const StructureNode& op1, const StructureNode& op2) {
			return op1.represents.id == op2.represents.id;
		}

		inline bool operator!=(const StructureNode& op1, const StructureNode& op2) { return !(op1 == op2); }

		/// Representation of tree branch made up from StructureNode.
		struct StructureBranch {
			/// Constructs new StructureBranch object.
			/// \param branchBase Branching node or root of tree.
			/// \param branchTip Tip of meshed branch.
			/// \param isTrunk Flag - if branch is trunk of tree.
			explicit StructureBranch(const rwStructNode branchBase, const rwStructNode branchTip, const bool isTrunk) :
				branchBase{branchBase}, branchTip{branchTip}, isTrunk{isTrunk} {}

			rwStructNode branchBase;
			rwStructNode branchTip;

			bool isTrunk;
		};

		/// Meshed branch representation.
		/// Nodes of branch in its containers are in order from base to tip.
		struct MeshBranch {
			/// Constructs new MeshBranch object.
			/// \param seed Seed for random generators used during making of branch mesh.
			/// \param isTrunk Flag - if branch is trunk of tree.
			explicit MeshBranch(const unsigned seed, const bool isTrunk = false) : rndEngine{seed}, isTrunk{isTrunk} {}

			/// Calculates spline interpolating nodes of branch.
			void calculateSpline();

			/// Interpolates radii of branch nodes. Smoothes change of radius along the branch.
			void interpolateRadii();

			/// Collects mesh vertices and triangles of branch into container.
			/// \param output Container into which is mesh of branch collected.
			/// \warning Calling this method deletes mesh vertices of branch.
			void collectBranchMesh(TreeMesh& output);

			/// Deletes all mesh vertices.
			void clearTreeMesh();

			std::minstd_rand rndEngine;

			/// \name Branch node positions, radii, interpolation and other properties for making of mesh.
			///@{
			std::vector<glm::vec3> structNodesPositions{};
			std::vector<float> structNodesRadii{};
			bool isTrunk;
			std::unique_ptr<Spline3D::CubicSpline3D> spline = nullptr;
			///@}

			/// \name Branch mesh vertices.
			///@{
			std::unique_ptr<MeshVertex> meshTipVertex = nullptr;
			std::vector<std::vector<std::unique_ptr<MeshVertex>>> meshNodes{};
			///@}
		};
	}
}

namespace std
{
	/// std::hash overload for StructureNode.
	template <>
	struct hash<Treegen::MeshMaker::StructureNode> {
		/// Hash method of StructureNode.
		/// \param key StructureNode to be hashed.
		/// \returns Hash value of key.
		size_t operator()(const Treegen::MeshMaker::StructureNode& key) const;
	};
}


#endif // MESH_MAKER_DATA_TYPES_H
