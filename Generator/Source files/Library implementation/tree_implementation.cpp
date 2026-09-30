/// Tree representation implementation.
/// \file tree_implementation.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>

#include <Library implementation/tree_implementation.h>
#include <Library implementation/environment.h>

#include <Tree primitives/tree_primitives.h>
#include <Phyllotaxis/abstract_phyllotaxis.h>
#include <Signaling/ext_borchert_honda.h>
#include <Phyllotaxis/general_phyllotaxis.h>
#include <Shedding/shedding.h>
#include <Mesh maker/mesh_maker.h>

namespace Treegen
{
	TreeImplementation::TreeImplementation(const unsigned treeID, const glm::vec3& rootPosition,
	                                       Environment environment) :
		mTreeId{treeID}, mEnvironment{std::move(environment)}, mRootScenePosition{rootPosition} {
		mShedding = std::make_unique<Shedding::Shedding>(mEnvironment.shadow);
		mResourcesModel = std::make_unique<Signaling::ExtBorchertHondaModel>();
		mPhyllotaxis = std::make_unique<Phyllotaxis::GeneralPhyllotaxis>();
		mMeshMaker = std::make_unique<MeshMaker::MeshMaker>();
	}

	// ===========================================================================================================
	// Getters - model
	// ===========================================================================================================

	unsigned TreeImplementation::randomSeed() const { return mTreeRndSeed; }
	unsigned TreeImplementation::treeId() const { return mTreeId; }

	bool TreeImplementation::applyTropismToTrunk() const { return mApplyTropismToTrunk; }
	float TreeImplementation::trunkStraightness() const { return mTrunkStraightness; }
	float TreeImplementation::bareTrunkLength() const { return mBareTrunkLength; }
	float TreeImplementation::maxShootLength() const { return mMaxShootLength; }

	float TreeImplementation::biasToMain() const {
		return dynamic_cast<Signaling::ExtBorchertHondaModel&>(*mResourcesModel).biasToMain();
	}

	PhyllotacticType TreeImplementation::phyllotacticType() const {
		const auto type = dynamic_cast<Phyllotaxis::GeneralPhyllotaxis&>(*mPhyllotaxis).type();
		if (type == Phyllotaxis::GeneralPhyllotaxis::Type::Alternate) {
			return PhyllotacticType::Alternate;
		}
		return PhyllotacticType::Opposite;
	}

	float TreeImplementation::phyllotacticAngle() const { return mPhyllotaxis->phyllotacticAngle(); }
	float TreeImplementation::lateralAngle() const { return mPhyllotaxis->lateralAngle(); }

	float TreeImplementation::gravimorphismHorizontal() const { return mGravimorphismHorizontal; }
	float TreeImplementation::gravimorphismVertical() const { return mGravimorphismVertical; }
	float TreeImplementation::gravimorphismUpDown() const { return mGravimorphismUpDown; }
	bool TreeImplementation::applyShedding() const { return mApplyShedding; }

	float TreeImplementation::sheddingThreshold() const { return mShedding->sheddingThreshold(); }
	float TreeImplementation::lightSensitivity() const { return mLightSensitivity; }

	float TreeImplementation::budDirectionWeight() const { return mBudDirWeight; }
	float TreeImplementation::spaceDirectionWeight() const { return mSpaceDirWeight; }
	float TreeImplementation::tropismDirectionWeight() const { return mTropismWeight; }
	float TreeImplementation::tropismAngle() const { return mTropismAngle; }

	// ===========================================================================================================
	// Getters - mesh
	// ===========================================================================================================

	float TreeImplementation::branchThickness() const { return mMeshMaker->branchThickness(); }
	float TreeImplementation::pipeModelN() const { return mMeshMaker->pipeModelN(); }
	size_t TreeImplementation::subinternodes() const { return mMeshMaker->subinternodes(); }
	size_t TreeImplementation::verticesOnCircumference() const { return mMeshMaker->verticesInNode(); }
	size_t TreeImplementation::textureWidth() const { return mMeshMaker->textureWidth(); }
	size_t TreeImplementation::textureHeight() const { return mMeshMaker->textureHeight(); }
	float TreeImplementation::textureInternodesCovered() const { return mMeshMaker->textureInternodesCovered(); }

	// ===========================================================================================================
	// Setters - model
	// ===========================================================================================================

	void TreeImplementation::randomSeed(const unsigned seed) { mTreeRndSeed = seed; }

	void TreeImplementation::bareTrunkLength(const float length) {
		if (length < 0.f) { throw std::invalid_argument{"Bare trunk length must not be negative!"}; }
		mBareTrunkLength = length;
	}

	void TreeImplementation::maxShootLength(const float length) {
		if (length <= 0.f) { throw std::invalid_argument{"Max shoot length must be positive!"}; }
		mMaxShootLength = length;
	}

	void TreeImplementation::biasToMain(const float bias) {
		auto newModel = std::make_unique<Signaling::ExtBorchertHondaModel>(bias);
		mResourcesModel = std::move(newModel);
	}

	void TreeImplementation::phyllotacticType(const PhyllotacticType type) {
		const auto targetType = type == PhyllotacticType::Alternate
			                        ? Phyllotaxis::GeneralPhyllotaxis::Type::Alternate
			                        : Phyllotaxis::GeneralPhyllotaxis::Type::Opposite;
		dynamic_cast<Phyllotaxis::GeneralPhyllotaxis&>(*mPhyllotaxis).type(targetType);
	}

	void TreeImplementation::phyllotacticAngle(const float angleRadians) {
		mPhyllotaxis->phyllotacticAngle(angleRadians);
	}

	void TreeImplementation::lateralAngle(const float angleRadians) {
		mPhyllotaxis->lateralAngle(angleRadians);
	}

	void TreeImplementation::gravimorphismHorizontal(const float value) {
		if (value < 0.1f || value > 1.f) {
			throw std::invalid_argument{"Gravimorphism horizontal must be in range [0.1, 1.0]!"};
		}
		mGravimorphismHorizontal = value;
	}

	void TreeImplementation::gravimorphismVertical(const float value) {
		if (value < 0.1f || value > 1.f) {
			throw std::invalid_argument{"Gravimorphism vertical must be in range [0.1, 1.0]!"};
		}
		mGravimorphismVertical = value;
		mGravimorphismDisplacement = mGravimorphismUpDown * value;
	}

	void TreeImplementation::gravimorphismUpDown(const float upDown) {
		if (upDown < -0.9f || upDown > 0.9) {
			throw std::invalid_argument{"Gravimorphism up/down must be in range [-0.9, 0.9]!"};
		}
		mGravimorphismUpDown = upDown;
		mGravimorphismDisplacement = mGravimorphismVertical * upDown;
	}

	void TreeImplementation::applyShedding(const bool applyShedding) {
		mApplyShedding = applyShedding;
	}

	void TreeImplementation::sheddingThreshold(const float threshold) {
		mShedding->sheddingThreshold(threshold);
	}

	void TreeImplementation::lightSensitivity(const float sensitivity) {
		if (sensitivity < 0.f) { throw std::invalid_argument{"Light sensitivity must not be in negative!"}; }
		mLightSensitivity = sensitivity;
	}

	void TreeImplementation::budDirectionWeight(const float weight) {
		if (weight < 0.f) { throw std::invalid_argument{"Bud direction weight must not be negative!"}; }
		mBudDirWeight = weight;
	}

	void TreeImplementation::spaceDirectionWeight(const float weight) {
		if (weight < 0.f) { throw std::invalid_argument{"Space direction weight must not be negative!"}; }
		mSpaceDirWeight = weight;
	}

	void TreeImplementation::tropismDirectionWeight(const float weight) {
		if (weight < 0.f) { throw std::invalid_argument{"Tropism direction weight must not be negative!"}; }
		mTropismWeight = weight;
	}

	void TreeImplementation::tropismAngle(const float angleRadians) {
		if (angleRadians < 0.f || angleRadians > glm::radians(180.f)) {
			throw std::invalid_argument{"Tropism angle must be in range [0.0, pi]!"};
		}
		mTropismAngle = angleRadians;
	}

	void TreeImplementation::applyTropismToTrunk(const bool apply) { mApplyTropismToTrunk = apply; }

	void TreeImplementation::trunkStraightness(const float straightness) {
		if (straightness < 0.f || straightness > 1.f) {
			throw std::invalid_argument{"Trunk straightness must be in range [0.0, 1.0]!"};
		}
		mTrunkStraightness = straightness;
	}

	// ===========================================================================================================
	// Setters - mesh
	// ===========================================================================================================

	void TreeImplementation::branchThickness(const float thickness) { mMeshMaker->branchThickness(thickness); }
	void TreeImplementation::pipeModelN(const float n) { mMeshMaker->pipeModelN(n); }

	void TreeImplementation::subinternodes(const size_t subinternodes) {
		mMeshMaker->subinternodes(subinternodes);
	}

	void TreeImplementation::verticesOnCircumference(const size_t vertices) {
		mMeshMaker->verticesInNode(vertices);
	}

	void TreeImplementation::textureSizes(const size_t width, const size_t height) {
		mMeshMaker->textureSizes(width, height);
	}

	void TreeImplementation::textureInternodesCovered(const float internodesCovered) {
		mMeshMaker->textureInternodesCovered(internodesCovered);
	}

	// ===========================================================================================================
	// Model observation
	// ===========================================================================================================

	glm::vec3 TreeImplementation::rootScenePosition() const {
		return mRootScenePosition;
	}

	size_t TreeImplementation::nodesCount() const { return mTreeNodes.size(); }

	void TreeImplementation::cancelMakeMesh() {
		mMeshMaker->cancel();
	}

	TreeStructure TreeImplementation::getTreeStructure() {
		return collectBranchesStructure(std::begin(mBranchesAlive), std::end(mBranchesAlive));
	}

	TreeStructure TreeImplementation::getShedBranches() {
		return collectBranchesStructure(std::begin(mBranchesShed), std::end(mBranchesShed));
	}

	TreeMesh TreeImplementation::makeTreeMesh() {
		if (!mRoot) { return TreeMesh{mTreeId}; }
		mMeshMaker->rndSeed(mTreeRndSeed);
		mMeshMaker->sceneModelRatio(mEnvironment.scene->sceneModelRatio());
		auto treeMesh = mMeshMaker->makeMesh(*mRoot, mTreeId);

		tbb::parallel_for(tbb::blocked_range<size_t>(0, treeMesh.vertices.size()),
			[&](const tbb::blocked_range<size_t>& range) {
				for (auto i = range.begin(); i != range.end(); ++i) {
					treeMesh.vertices[i]->position =
						mEnvironment.scene->getSceneFromModelPos(treeMesh.vertices[i]->position);
				}
			});

		return treeMesh;
	}


	// ===========================================================================================================
	// Algorithm support methods
	// ===========================================================================================================

	void TreeImplementation::clearModel(const bool removeFromEnvironment) {
		if (removeFromEnvironment) {
			for (auto&& uqIdNodePair : mTreeNodes) {
				TreeNode& node = *uqIdNodePair.second;
				if (!node.shed) {
					mEnvironment.space->removeTreeNode(node);
					mEnvironment.shadow->removeTreeNode(node);
				}
			}
		}

		mRoot = nullptr;
		mNewTreeNodeBranchId = 0;
		mTreeNodes.clear();
		mTreeBranches.clear();
		mBranchesAlive.clear();
		mBranchesShed.clear();
		mActiveBuds.clear();
		mLastLayerNodes.clear();
		mLastShootNewNodes.clear();
		mPhyllotaxis->clear();
	}

	void TreeImplementation::initModel() {
		if (mRoot) { return; }

		const auto rootModelPos = mEnvironment.scene->getModelFromScenePos(mRootScenePosition);
		if (!mEnvironment.scene->isInBoundingBox(rootModelPos)) { return; }

		auto root = std::make_unique<TreeNode>(mNewTreeNodeBranchId, rootModelPos);
		auto trunk = std::make_unique<TreeBranch>(mNewTreeNodeBranchId, *root, *root);
		++mNewTreeNodeBranchId;

		mRoot = root.get();
		mPhyllotaxis->generateRootBuds(*root, mTreeRndSeed);
		mEnvironment.shadow->addTreeNode(*root);
		mEnvironment.space->addTreeNode(*root);

		const auto rootBuds = root->getBuds();	// No move assign - (active buds have more memory reserved)
		mActiveBuds.insert(std::end(mActiveBuds), std::begin(rootBuds), std::end(rootBuds));
		mBranchesAlive.insert(*trunk);

		mTreeNodes.emplace(root->id, std::move(root));
		mTreeBranches.emplace(trunk->id, std::move(trunk));
	}

	bool TreeImplementation::hasActiveBuds() const { return !mActiveBuds.empty(); }

	void TreeImplementation::applyEnvironmentResources() {
		if (mActiveBuds.empty()) { return; }

		tbb::parallel_for(tbb::blocked_range<size_t>(0, mActiveBuds.size()),
			[&](const tbb::blocked_range<size_t>& range) {
				for (auto i = range.begin(); i != range.end(); ++i) {
					TreeBud& bud = mActiveBuds[i];
					mEnvironment.shadow->assignResources(bud);
					bud.resources = std::pow(bud.resources, mLightSensitivity);
					if (bud.type == BudType::Lateral) { applyGravimorphism(bud); }
				}
			});

		mResourcesModel->redistributeResources(*mRoot, mActiveBuds);
		shootLengthCalculation();
	}

	void TreeImplementation::pushBudsToSpace() {
		mEnvironment.space->addTreeBuds(std::begin(mActiveBuds), std::end(mActiveBuds));
	}

	void TreeImplementation::handleSpaceCalculationResults() {
		mActiveBuds.erase(std::remove_if(std::begin(mActiveBuds), std::end(mActiveBuds), [](TreeBud& bud) {
			return bud.status == BudStatus::NoSpace;
		}), std::end(mActiveBuds));
	}

	void TreeImplementation::growShootNode() {
		// Remove buds with no space
		mActiveBuds.erase(std::remove_if(std::begin(mActiveBuds), std::end(mActiveBuds), [](TreeBud& bud) {
			return bud.status != BudStatus::Active;
		}), std::end(mActiveBuds));

		growBuds();

		// Terminal buds of new nodes inherited (grow length - 1)
		mActiveBuds.clear();
		for (auto&& rwNode : mLastLayerNodes) {
			TreeNode& node = rwNode;
			if (node.terminalBud.get()->shootLength != 0) { mActiveBuds.push_back(*node.terminalBud); }
		}

		mLastShootNewNodes.insert(std::end(mLastShootNewNodes),
			std::begin(mLastLayerNodes), std::end(mLastLayerNodes));
		mLastLayerNodes.clear();
	}

	void TreeImplementation::shedBranches() {
		if (!mApplyShedding) { return; }

		const auto shedBranches = mShedding->shedBranches(std::begin(mBranchesAlive), std::end(mBranchesAlive));
		for (auto&& rwShedBranch : shedBranches) { mBranchesAlive.unsafe_erase(rwShedBranch); }
		
		// Nodes must be removed from shadow propagation in deterministic order
		// otherwise floating point rounding would be able to change outcome
		tbb::concurrent_vector<rwTreeNode> nodesToRemove{};

		tbb::parallel_for(tbb::blocked_range<size_t>(0, shedBranches.size()),
			[&](const tbb::blocked_range<size_t>& range) {
				for (auto i = range.begin(); i != range.end(); ++i) {
					TreeBranch& branch = shedBranches[i];

					branch.status = BranchStatus::Shed;
					mBranchesShed.insert(branch);

					// Mark nodes as shed and remove their active buds
					auto rwCurrent = branch.tipNode;
					while (rwCurrent != branch.baseNode) {
						TreeNode& currentNode = rwCurrent;

						currentNode.shed = true;
						nodesToRemove.push_back(currentNode);
						mEnvironment.space->removeTreeNode(currentNode);
						auto buds = currentNode.getBuds();
						for (auto&& bud : buds) { bud.get().status = BudStatus::Shed; }

						rwCurrent = *currentNode.predecessor; // Branch should have connected nodes from tip to base
					}
				}
			});

		std::sort(std::begin(nodesToRemove), std::end(nodesToRemove));
		for (auto&& rwNode : nodesToRemove) { mEnvironment.shadow->removeTreeNode(rwNode); }
	}

	void TreeImplementation::collectValidShootBuds() {
		// Active buds for next iteration (we need to clear buds that were internodes of shoots)
		mActiveBuds.clear();
		for (auto&& rwNode : mLastShootNewNodes) {
			const auto buds = rwNode.get().getBuds();
			for (auto&& rwBud : buds) {
				if (rwBud.get().status == BudStatus::Active) { mActiveBuds.push_back(rwBud); }
			}
		}
		mLastShootNewNodes.clear();
	}


	// ===========================================================================================================
	// Private methods
	// ===========================================================================================================

	void TreeImplementation::applyGravimorphism(TreeBud& lateralBud) {
		assert(lateralBud.type == BudType::Lateral);

		// Angle of branch in respect to vertical orientation
		const auto heading = lateralBud.owner.get().terminalBud.get()->orientationNormalized;
		const auto cosAngle = glm::dot(heading, Const::WORLD_UP);

		// Branch is vertical => gravimorphism does not influence it
		if (std::abs(cosAngle) > Const::COS_1_DEG) { return; }

		const auto cosPow2 = cosAngle * cosAngle;
		const auto sinPow2 = 1.f - cosPow2;

		// Ellipsis plane perpendicular to heading of branch
		const auto parallel = glm::normalize(glm::cross(heading, Const::WORLD_UP));
		const auto vertical = glm::normalize(glm::cross(parallel, heading));

		// Projection of bud orientation on ellipsis plane
		const auto inEllipsePlane = glm::normalize(
			lateralBud.orientationNormalized - glm::dot(lateralBud.orientationNormalized, heading) * heading);

		const float distanceToEllipsis = [&]() {
			auto isSameSign = [](const float op1, const float op2) { return op1 * op2 >= 0.f; };

			// Get coefficient of projection on parallel and vertical => x, y parameters
			// If necessary calculate intersection of line and ellipsis (moved by Up/Down from center)

			// Line
			const auto xProjected = glm::dot(inEllipsePlane, parallel);
			const auto yProjected = glm::dot(inEllipsePlane, vertical);

			if (yProjected == 0.f) { return mGravimorphismHorizontal; }
			if (xProjected == 0.f) {
				if (yProjected >= 0.f) { return mGravimorphismVertical + mGravimorphismDisplacement; }
				else { return mGravimorphismVertical - mGravimorphismDisplacement; }
			}

			const auto m = yProjected / xProjected;
			const auto mPow2 = m * m;

			// Ellipsis
			const auto a = mGravimorphismHorizontal;
			const auto b = mGravimorphismVertical;
			const auto k = mGravimorphismDisplacement;
			const auto e = -k;

			const auto aPow2 = a * a;
			const auto bPow2 = b * b;
			const auto kPow2 = k * k;

			// Intersection calculation
			const auto denominator = aPow2 * mPow2 + bPow2;
			const auto disSqrt = std::sqrt(denominator - kPow2);

			const auto xNumeratorLeft = -m * aPow2 * e;
			const auto xNumeratorRight = a * b * disSqrt;

			const auto yNumeratorLeft = k * aPow2 * mPow2;
			const auto yNumeratorRight = a * b * m * disSqrt;

			// Intersection points => determine correct point based on x/y +- signs (and orientation of direction)
			auto intersectionX = (xNumeratorLeft + xNumeratorRight) / denominator;
			auto intersectionY = (yNumeratorLeft + yNumeratorRight) / denominator;

			if (!isSameSign(intersectionX, xProjected) || !isSameSign(intersectionY, yProjected)) {
				intersectionX = (xNumeratorLeft - xNumeratorRight) / denominator;
				intersectionY = (yNumeratorLeft - yNumeratorRight) / denominator;
			}

			return glm::length(glm::vec2{intersectionX, intersectionY});
		}();

		const auto gravimorphicCoefficient = cosPow2 + distanceToEllipsis * sinPow2;
		lateralBud.resources *= gravimorphicCoefficient;
	}

	void TreeImplementation::shootLengthCalculation() {
		assert(!mActiveBuds.empty());

		// Max bud resources in iteration
		const auto maxResources = std::max_element(std::begin(mActiveBuds), std::end(mActiveBuds),
			[](TreeBud& a, TreeBud& b) {
				return a.resources < b.resources;
			})->get().resources;

		// Assign shoot length based on resources and maximum growth length
		for (auto&& rwBud : mActiveBuds) {
			TreeBud& bud = rwBud;
			// Max resources divided by (mMaxShootLength + 1) intervals (growth length = 0, ..., mMaxShootLength)
			// Otherwise only buds with maxResources would have shoots equal to maximal length => add small constant to max
			bud.shootLength = int((bud.resources / (maxResources + 0.001f)) * (mMaxShootLength + 1));
			bud.resources = 0.f;
			if (bud.shootLength == 0) { bud.status = BudStatus::NoLight; }
		}

		// Clear buds that dont have enough resources for grow (so they wont take part in shoot growth)
		mActiveBuds.erase(std::remove_if(std::begin(mActiveBuds), std::end(mActiveBuds), [](TreeBud& bud) {
			return bud.status == BudStatus::NoLight;
		}), std::end(mActiveBuds));
	}

	void TreeImplementation::growBuds() {
		// For reproductibility of model based on seed we need to add new nodes
		// into shadow propagation in deterministic order
		// new nodes created in parallel execution must have deterministic id
		// => we premake potential id
		// otherwise floating point rounding would change result
		const auto baseId = mNewTreeNodeBranchId;
		mNewTreeNodeBranchId += unsigned(mActiveBuds.size());

		tbb::parallel_for(tbb::blocked_range<size_t>(0, mActiveBuds.size()),
			[&](const tbb::blocked_range<size_t>& range) {
				for (auto i = range.begin(); i != range.end(); ++i) {
					TreeBud& bud = mActiveBuds[i];
					const auto potentialNode = tryGrowBud(bud, baseId + unsigned(i));
					if (potentialNode) {
						potentialNode->terminalBud.get()->shootLength = bud.shootLength - 1;
						mLastLayerNodes.push_back(*potentialNode);
						mEnvironment.space->addTreeNode(*potentialNode);
					}
					bud.shootLength = 0;
				}
			});

		std::sort(std::begin(mLastLayerNodes), std::end(mLastLayerNodes));
		for (auto&& rwNode : mLastLayerNodes) { mEnvironment.shadow->addTreeNode(rwNode); }
	}

	TreeNode* TreeImplementation::tryGrowBud(TreeBud& fromBud, const unsigned id) {
		// Position of following node is at: base + calc_prefered_dir * internode length
		const auto& fromPosition = fromBud.owner.get().position;
		const auto budGrowthDirection = calcBudGrowthDir(fromBud);
		const auto growthDirLength = glm::length(budGrowthDirection);
		if (growthDirLength == 0.f) { return nullptr; }

		// Stop bud from growing if tree node would be outside outside of bounding box or in collision with scene object
		if (!mEnvironment.scene->isValidDirection(fromPosition, budGrowthDirection, 1.f)) {
			fromBud.status = BudStatus::SceneOrBoundsCollision;
			return nullptr;
		}

		const auto newNodePosition = fromPosition + (budGrowthDirection / growthDirLength);
		return &growFromBud(fromBud, newNodePosition, id).get();
	}

	glm::vec3 TreeImplementation::calcBudGrowthDir(TreeBud& fromBud) {
		const bool isTrunk = fromBud.type == BudType::Terminal && fromBud.owner.get().branch->isTrunk();

		glm::vec3 finalDir = fromBud.orientationNormalized * mBudDirWeight;
		finalDir += fromBud.getFreeSpaceDirNormalized() * mSpaceDirWeight;

		if (isTrunk) { finalDir.y += mTrunkStraightness; }

		if (!isTrunk || mApplyTropismToTrunk) {
			const auto rotationAxis = [&]() {
				const auto cosAngle = glm::dot(Const::WORLD_UP, fromBud.orientationNormalized);
				if (std::abs(cosAngle) > Const::COS_1_DEG) {
					return Utils::getRndOrthogonalNormalized(fromBud.orientationNormalized, fromBud.rndEngine);
				}
				return glm::cross(Const::WORLD_UP, fromBud.orientationNormalized);
			}();

			const auto& tropismDir = GlmUtils::rotateAroundAxis(Const::WORLD_UP, mTropismAngle, rotationAxis);
			finalDir += tropismDir * mTropismWeight;
		}

		return finalDir;
	}

	rwTreeNode TreeImplementation::growFromBud(TreeBud& fromBud, const glm::vec3& position, const unsigned id) {
		auto uqNewNode = std::make_unique<TreeNode>(id, position);

		TreeNode& newNode = *uqNewNode;
		TreeNode& predecessor = fromBud.owner;

		fromBud.status = BudStatus::MadeBranch;
		newNode.predecessor = &predecessor;

		if (fromBud.type == BudType::Terminal) {
			// Terminal bud is only one => there can not be race condition on assigning follower and growing branch
			predecessor.terminalFollower = &newNode;
			predecessor.branch->growWith(newNode);
		}
		else {
			// Inserting data to concurrent containers
			auto uqNewBranch = std::make_unique<TreeBranch>(id, predecessor, newNode);
			mBranchesAlive.insert(*uqNewBranch);
			mTreeBranches.emplace(uqNewBranch->id, std::move(uqNewBranch));
			
			// Synchronization needed - for multiple lateral followers
			std::lock_guard<std::mutex> lock{predecessor.nodeMutex};
			predecessor.lateralFollowers.push_back(newNode);
		}

		// Phyllotaxis applied after predecessor is defined!
		mPhyllotaxis->generateInnerBuds(newNode, fromBud);
		if (newNode.branch->isTrunk() && newNode.branch->nodes.size() - 1 <= mBareTrunkLength) {
			newNode.lateralBuds.clear();
		}

		mTreeNodes.emplace(uqNewNode->id, std::move(uqNewNode));
		return newNode;
	}
}
