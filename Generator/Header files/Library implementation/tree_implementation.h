/// Tree representation implementation.
/// \file tree_implementation.h
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#ifndef TREE_IMPLEMENTATION_H
#define TREE_IMPLEMENTATION_H

#include <Library interface/tree.h>
#include <Library implementation/environment.h>

#include <Tree primitives/tree_primitives.h>
#include <Signaling/abstract_resources_model.h>
#include <Phyllotaxis/abstract_phyllotaxis.h>
#include <Shedding/shedding.h>
#include <Mesh maker/mesh_maker.h>

#include <glm/vec3.hpp>

#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/concurrent_unordered_map.h>

#include <numeric>
#include <vector>
#include <memory>

namespace Treegen
{
	class TreeImplementation;
	using rwTreeImpl = std::reference_wrapper<TreeImplementation>;
	using rwConstTreeImpl = std::reference_wrapper<const TreeImplementation>;

	/// Tree representation implementation.
	class TreeImplementation final : public Tree {
		
		/// \name Tree properties
		///@{
		const unsigned mTreeId;
		unsigned mTreeRndSeed = Utils::getRndSeed();
		unsigned mNewTreeNodeBranchId = 0;	///< Id for new branch is equal to id of node starting it.

		bool mApplyTropismToTrunk = false;
		float mTrunkStraightness = 0.f;
		float mBareTrunkLength = 0.f;
		float mMaxShootLength = 2.f;

		float mGravimorphismHorizontal = 1.f;
		float mGravimorphismVertical = 1.f;
		float mGravimorphismUpDown = 0.f;
		float mGravimorphismDisplacement = 0.f;

		bool mApplyShedding = true;
		float mLightSensitivity = 1.f;

		float mBudDirWeight = 1.f;
		float mSpaceDirWeight = 1.f;
		float mTropismWeight = 0.f;
		float mTropismAngle = 0.f;
		///@}

		/// \name Classess necessary for simulation of growth and making of mesh.
		///@{
		Environment mEnvironment;
		std::unique_ptr<Signaling::AbstractResourcesModel> mResourcesModel = nullptr;
		std::unique_ptr<Phyllotaxis::AbstractPhyllotaxis> mPhyllotaxis = nullptr;
		std::unique_ptr<Shedding::Shedding> mShedding = nullptr;
		std::unique_ptr<MeshMaker::MeshMaker> mMeshMaker = nullptr;
		///@}

		glm::vec3 mRootScenePosition{0.f};

		/// \name Data representing tree structure.
		///@{
		TreeNode* mRoot = nullptr;
		tbb::concurrent_unordered_map<unsigned, std::unique_ptr<TreeNode>> mTreeNodes{};
		tbb::concurrent_unordered_map<unsigned, std::unique_ptr<TreeBranch>> mTreeBranches{};
		tbb::concurrent_unordered_set<rwTreeBranch, std::hash<TreeBranch>> mBranchesAlive{};
		tbb::concurrent_unordered_set<rwTreeBranch, std::hash<TreeBranch>> mBranchesShed{};

		tbb::concurrent_vector<rwTreeNode> mLastLayerNodes{};
		std::vector<rwTreeNode> mLastShootNewNodes{};

		std::vector<rwTreeBud> mActiveBuds{};
		///@}

	public:

		/// Constructs new TreeImplementation object.
		/// \param treeID Identifier of tree.
		/// \param rootPosition Position of root of tree.
		/// \param environment Environment in which tree will grow.
		explicit TreeImplementation(unsigned treeID, const glm::vec3& rootPosition, Environment environment);
		
		/// \name Public API implementation.
		///@{

		// ===========================================================================================================
		// Getters - model
		// ===========================================================================================================

		unsigned randomSeed() const override;
		unsigned treeId() const override;

		float bareTrunkLength() const override;
		float maxShootLength() const override;
		float biasToMain() const override;

		PhyllotacticType phyllotacticType() const override;
		float phyllotacticAngle() const override;
		float lateralAngle() const override;

		float gravimorphismHorizontal() const override;
		float gravimorphismVertical() const override;
		float gravimorphismUpDown() const override;

		bool applyShedding() const override;
		float sheddingThreshold() const override;
		float lightSensitivity() const override;

		float budDirectionWeight() const override;
		float spaceDirectionWeight() const override;
		float tropismDirectionWeight() const override;
		float tropismAngle() const override;
		bool applyTropismToTrunk() const override;
		float trunkStraightness() const override;

		// ===========================================================================================================
		// Getters - mesh
		// ===========================================================================================================

		size_t subinternodes() const override;
		size_t verticesOnCircumference() const override;
		float branchThickness() const override;
		float pipeModelN() const override;
		size_t textureWidth() const override;
		size_t textureHeight() const override;
		float textureInternodesCovered() const override;

		// ===========================================================================================================
		// Setters - model
		// ===========================================================================================================

		void randomSeed(unsigned seed) override;
		void bareTrunkLength(float length) override;
		void maxShootLength(float length) override;
		void biasToMain(float bias) override;

		void phyllotacticType(PhyllotacticType type) override;
		void phyllotacticAngle(float angleRadians) override;
		void lateralAngle(float angleRadians) override;

		void gravimorphismHorizontal(float value) override;
		void gravimorphismVertical(float value) override;
		void gravimorphismUpDown(float upDown) override;

		void applyShedding(bool applyShedding) override;
		void sheddingThreshold(float threshold) override;
		void lightSensitivity(float sensitivity) override;

		void budDirectionWeight(float weight) override;
		void spaceDirectionWeight(float weight) override;
		void tropismDirectionWeight(float weight) override;
		void tropismAngle(float angleRadians) override;
		void applyTropismToTrunk(bool isStraight) override;
		void trunkStraightness(float straightness) override;

		// ===========================================================================================================
		// Setters - mesh
		// ===========================================================================================================

		void branchThickness(float thickness) override;
		void pipeModelN(float n) override;
		void subinternodes(size_t subinternodes) override;
		void verticesOnCircumference(size_t vertices) override;
		void textureSizes(size_t width, size_t height) override;
		void textureInternodesCovered(float internodesCovered) override;

		// ===========================================================================================================
		// Model observation
		// ===========================================================================================================

		glm::vec3 rootScenePosition() const override;
		size_t nodesCount() const override;

		void cancelMakeMesh() override;
		TreeStructure getTreeStructure() override;
		TreeStructure getShedBranches() override;
		TreeMesh makeTreeMesh() override;

		///@}

		// ===========================================================================================================
		// Algorithm methods
		// ===========================================================================================================

		/// \name Generation cycle methods.
		///@{

		/// Clears generated tree model - equal to state before start of generation.
		/// \param removeFromEnvironment Flag - if tree data should be removed from envrionment representation.
		/// Usable when single tree is deleted.
		void clearModel(bool removeFromEnvironment);

		/// Initializes model before run of generative algorithm.
		void initModel();

		/// Checks if tree has any active buds capable of growth.
		/// \return Resukt of check.
		bool hasActiveBuds() const;

		/// Applies resources from light and defines length of new shoots to be grown.
		void applyEnvironmentResources();

		/// Sends bud capable of growing of shoots into space colonization algorithm.
		void pushBudsToSpace();

		/// Handles result of space colonization algorithm.
		void handleSpaceCalculationResults();

		/// Grows one tree node on all valid shoots.
		void growShootNode();

		/// Sheds branches.
		void shedBranches();

		/// Collects valid buds for growth in next iteration from nodes of new shoots.
		void collectValidShootBuds();

		///@}

	private:
		/// \name Generation sub/supporting methods.
		///@{

		/// Collects representation of tree structure from branches.
		/// \tparam TIterator Iterator of TreeBranch objects.
		/// \param begin Begin iterator.
		/// \param end End iterator.
		/// \return Representation of branches.
		template <typename TIterator>
		TreeStructure collectBranchesStructure(TIterator begin, TIterator end);

		/// Applies gravimorphic bias to lateral bud.
		void applyGravimorphism(TreeBud& lateralBud);

		/// Assigns lengths of shoots to buds.
		void shootLengthCalculation();

		/// Grows new nodes from all active buds (buds capable of growth)
		void growBuds();

		/// Tries to grow new tree node from given bud (expects that bud has assigned prefered directions of shoot).
		/// \param fromBud Growing bud.
		/// \param id Identifier for new potential node and branch.
		/// \return Pointer to new node or nullptr if node could not be made because it was outside of valid space in scene.
		TreeNode* tryGrowBud(TreeBud& fromBud, unsigned id);

		/// Calculates final direction based on weighted values of bud orientation,  
		/// prefered direction (based on space) and tropism  value.
		/// \param fromBud Growing bud.
		/// \return Direction for growth of bud.
		glm::vec3 calcBudGrowthDir(TreeBud& fromBud);

		/// Grows new tree node from bud.
		/// Handles all references of new node inside tree structure.
		/// \param fromBud Growing bud.
		/// \param position Position of new tree node.
		/// \param id Identifier of new node.
		/// \return Reference to new node.
		rwTreeNode growFromBud(TreeBud& fromBud, const glm::vec3& position, unsigned id);

		///@}
	};

	template <typename TIterator>
	TreeStructure TreeImplementation::collectBranchesStructure(TIterator begin, TIterator end) {
		TreeStructure structure{mTreeId};

		const size_t totalNodes = std::accumulate(begin, end, size_t(0),
			[](size_t current, const TreeBranch& branch) {
				return current + branch.nodes.size();
			});

		structure.nodesPositions.reserve(totalNodes);
		structure.internodes.reserve(totalNodes * 2);

		for (auto it = begin; it != end; ++it) {
			TreeBranch& branch = *it;
			auto rwNode = branch.tipNode;
			while (rwNode != branch.baseNode) {
				TreeNode& node = rwNode;
				const auto nodeScaledPos = mEnvironment.scene->getSceneFromModelPos(node.position);
				const auto predNodeScaledPos = mEnvironment.scene->getSceneFromModelPos(node.predecessor->position);

				structure.nodesPositions.push_back(nodeScaledPos);
				structure.internodes.push_back(nodeScaledPos);
				structure.internodes.push_back(predNodeScaledPos);
				rwNode = *node.predecessor;
			}
		}

		if (mRoot) { structure.nodesPositions.push_back(mRoot->position); }

		return structure;
	}
}


#endif // TREE_IMPLEMENTATION_H
