/// Class for meshing of tree structure.
/// \file mesh_maker.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef MESH_MAKER_H
#define MESH_MAKER_H

#include <Mesh maker/mesh_maker_data_types.h>
#include <Library interface/output_data_types.h>
#include <Tree primitives/tree_primitives.h>
#include <Utilities/treegen_utils.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <tbb/concurrent_vector.h>

#include <vector>
#include <memory>
#include <mutex>

namespace Treegen
{
	namespace MeshMaker
	{
		/// Class for meshing of tree structure.
		class MeshMaker {
			/// Default number of vertices on circumference of cyllinder representing branch.
			static constexpr size_t DEF_VERTICES_IN_NODE = 8;

			/// \name Mesh making settings and derived helper variables.
			///@{
			unsigned mRndSeed = Utils::getRndSeed();

			float mSceneModelRatio = 1.f;
			float mPipeModelN = 2.f;
			/// Tip radius for radius propagation (so radius ^ pipeModelN does not decrease radius (for radius < 1.0))
			float const mTipRadius = 1.f;
			float mBranchRadiusMult = 0.01f; ///< Multiplier of calculated radius from propagation (overall thickness).

			size_t mSubinternodes = 1;
			size_t mVerticesInNode{DEF_VERTICES_IN_NODE};
			float mNodeRotationAngle = glm::two_pi<float>() / DEF_VERTICES_IN_NODE;

			glm::ivec2 mTexSizes = {1, 1};
			float mTexWhRatio = 1.f;
			float mTexInternodesCovered = 1.f;
			///@}

			/// \name Tree representation and mesh data structures.
			///@{
			unsigned mTreeId = 0;
			StructureNode* mRoot = nullptr;
			std::vector<std::unique_ptr<StructureNode>> mStructNodes{};
			std::vector<StructureBranch> mStructBranches{};
			std::vector<rwStructNode> mBranchingNodes{};
			std::vector<rwStructNode> mTipNodes{};

			tbb::concurrent_vector<MeshBranch> mMeshBranches{};
			///@}

			/// \name State and access synchronization variables.
			///@{
			std::mutex mRunCheckMutex{};
			bool mIsRunning = false;
			bool mIsCancelled = false;
			///@}

		public:
			/// Sets scene - model ratio.
			/// \param ratio Positive value representing new ratio.
			/// \warning For invalid argument throws std::invalid_argument exception.
			void sceneModelRatio(float ratio);
			float sceneModelRatio() const { return mSceneModelRatio; }

			/// Sets branch thickness.
			/// \param thickness Thickness of branch from range [0.001, 0.1].
			/// \warning For invalid argument throws std::invalid_argument exception.
			void branchThickness(float thickness);
			float branchThickness() const { return mBranchRadiusMult; }

			/// Sets pipe model n.
			/// \param n Pipe model n from range [1.0, 3.0].
			/// \warning For invalid argument throws std::invalid_argument exception.
			void pipeModelN(float n);
			float pipeModelN() const { return mPipeModelN; }

			/// Sets number of subinternodes.
			/// \param subinternodes Nonzero number of subinternodes.
			/// \warning For invalid argument throws std::invalid_argument exception.
			void subinternodes(size_t subinternodes);
			size_t subinternodes() const { return mSubinternodes; }

			/// Sets number of vertices on circumference of branch.
			/// \param verticesCount Even number of vertices (at least 4).
			/// \warning For invalid argument throws std::invalid_argument exception.
			void verticesInNode(size_t verticesCount);
			size_t verticesInNode() const { return mVerticesInNode; }

			/// Sets texture width and height for UV mapping.
			/// \param width Texture width.
			/// \param height Texture height.
			void textureSizes(size_t width, size_t height);
			size_t textureWidth() const { return mTexSizes.s; }
			size_t textureHeight() const { return mTexSizes.t; }

			/// Sets scaling of height of texture.
			/// \param internodesCovered How many internodes of tree should texture cover. Must be bigger than zero.
			/// \warning For invalid argument throws std::invalid_argument exception.
			void textureInternodesCovered(float internodesCovered);
			float textureInternodesCovered() const { return mTexInternodesCovered; }

			/// Sets seed of random number generator for meshing of branches.
			/// \param seed Seed of random number generator.
			void rndSeed(unsigned seed);
			unsigned rndSeed() const { return mRndSeed; }

			/// Makes mesh of tree.
			/// \param root Root of tree.
			/// \param treeId Identifier of tree.
			/// \return Vertices and triangles of tree mesh.
			TreeMesh makeMesh(rwConstTreeNode root, unsigned treeId);
			
			/// Cancels making of mesh.
			/// Mesh making is stopped on first check for cancellation.
			/// After cancelled run is finished object is in valid state for further use.
			void cancel();

		private:
			/// Checks if cancellation of mesh making was requested (synchronized with cancellation).
			bool isCancelledCheck();

			/// Cleanup if cancellation was accepted.
			/// \return Empty mesh object.
			TreeMesh returnOnCancel();

			/// Loads tree into internal meshing representation.
			/// Handles tree with only one node.
			/// \param root Root of tree.
			/// \param treeId Identifier of tree.
			void loadTreeStructure(rwConstTreeNode root, unsigned treeId);

			/// Initializes meshing data structures of tree.
			/// \param root Root of tree.
			void initStructures(const TreeNode& root);

			/// Initializes representation of tree structure.
			/// \param structureRoot Root of tree.
			void initTreeStructure(const TreeNode& structureRoot);
			
			/// Assigns radii to branches of tree.
			void assignRadii();
			
			/// Initializes tree branches based on assigned radii.
			void initStructureBranches();
			
			/// Removes invalid following axes from list (shed nodes) and sorts rest in order of priority of being terminal continuation of branch.
			/// \param followingAxes Axes following node in tree.
			/// \return Number of valid axes in list sorted at beginning of container.
			size_t validFollAxesSize(std::vector<rwStructNode>& followingAxes);
			
			/// Initializes representations of branches for meshing.
			void initMeshBranches();

			/// Makes mesh for each branch.
			void makeBranchMeshes();
			
			/// Makes mesh for branch.
			void makeBranchMesh(MeshBranch& branch);
			
			/// Sets texture coordinates of branch mesh.
			void setTextureCoordinates(MeshBranch& branch);
			
			/// Makes mesh of short branch (2 nodes length).
			void makeMeshShortBranch(MeshBranch& branch);
			
			/// Makes mesh of long branch (more than 2 nodes).
			void makeMeshLongBranch(MeshBranch& branch);
			
			/// Makes vertices of branch circumference at position.
			/// \param position Center of branch (circumference).
			/// \param tangentUp Tangent of branch at \p position. 
			/// Oriented in direction of branch from base to tip (upwards in tree structure).
			/// \param normal Normal to \p tangentUp which represents direction to first vertex on circumference from its center.
			/// \param radius Radius of branch.
			/// \return Vertices on circumference of given properties.
			std::vector<std::unique_ptr<MeshVertex>> makeVerticesAtPosition(
				const glm::vec3& position, const glm::vec3& tangentUp, const glm::vec3& normal, float radius) const;
			
			/// Collects meshes of individual branches.
			/// \return Meshes of all branches of tree.
			TreeMesh collectTreeMeshData();
			
			/// Sets properties of mesh vertices (normal and tangent).
			/// \param data Mesh data whose vertices are to be set.
			void calculateVertexProperties(const TreeMesh& data);

			/// Clears all data. Prepares MeshMaker for making of next mesh. 
			void clear();
		};
	}
}
#endif // MESH_MAKER_H
