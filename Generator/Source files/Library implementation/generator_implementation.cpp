/// Tree generator implementation.
/// \file generator_implementation.cpp
/// \author Vojtěch Tázlar (tazlarvojta\@centrum.cz)
/// \date 2018

#include <stdafx.h>
#include <Library implementation/generator_implementation.h>

namespace Treegen
{
	// ===========================================================================================================
	// Getters - environment
	// ===========================================================================================================

	bool GeneratorImplementation::isEnvironmentLocked() const {
		std::lock_guard<std::mutex> lock{mStateLock};
		return mEnvironmentLocked;
	}

	bool GeneratorImplementation::isRunning() const {
		std::lock_guard<std::mutex> lock{mStateLock};
		return mIsRunning;
	}

	bool GeneratorImplementation::isCanceled() const {
		std::lock_guard<std::mutex> lock{mStateLock};
		return mIsCancelled;
	}

	float GeneratorImplementation::sceneModelRatio() const {
		return mEnvironment.scene->sceneModelRatio();
	}

	float GeneratorImplementation::perceptionDistance() const {
		return mEnvironment.space->attractionDistance();
	}

	float GeneratorImplementation::killDistance() const {
		return mEnvironment.space->killDistance();
	}

	size_t GeneratorImplementation::budFreeSpaceMarkers() const {
		return mEnvironment.space->budFreeSpaceMarkers();
	}

	float GeneratorImplementation::coneAngle() const {
		return mEnvironment.space->coneAngle();
	}

	size_t GeneratorImplementation::shadowPyramidHeight() const {
		return mEnvironment.shadow->pyramidHeight();
	}

	float GeneratorImplementation::nodeShadow() const {
		return mEnvironment.shadow->nodeShadow();
	}

	float GeneratorImplementation::shadowDiminishMultiplier() const {
		return mEnvironment.shadow->diminishingSpeed();
	}

	float GeneratorImplementation::maximumLightValue() const {
		return mEnvironment.shadow->maxLight();
	}

	// ===========================================================================================================
	// Setters - environment
	// ===========================================================================================================

	void GeneratorImplementation::sceneModelRatio(const float ratio) {
		checkEnvironmentLockException();
		mEnvironment.scene->sceneModelRatio(ratio);
	}

	void GeneratorImplementation::perceptionOccupancyDistance(const float perceptionDistance,
	                                                          const float occupancyDistance) {
		checkEnvironmentLockException();
		mEnvironment.space->attractionKillDistance(perceptionDistance, occupancyDistance);
	}

	void GeneratorImplementation::budFreeSpaceMarkers(const size_t count) {
		checkEnvironmentLockException();
		mEnvironment.space->budFreeSpaceMarkers(count);
	}

	void GeneratorImplementation::coneAngle(const float angleRadians) {
		checkEnvironmentLockException();
		mEnvironment.space->coneAngle(angleRadians);
	}

	void GeneratorImplementation::setShadowProperties(const size_t pyramidHeight, const float nodeShadow,
	                                                  const float shadowDiminishMultiplier,
	                                                  const float maximumLight) {
		checkEnvironmentLockException();
		mEnvironment.shadow->setProperties(pyramidHeight, nodeShadow, shadowDiminishMultiplier, maximumLight);
	}

	// ===========================================================================================================
	// Tree && Scene API
	// ===========================================================================================================

	std::vector<unsigned> GeneratorImplementation::validTreeIds() const {
		std::vector<unsigned> ids{};
		ids.reserve(mTrees.size());
		for (auto&& idTreePair : mTrees) { ids.push_back(idTreePair.first); }
		return ids;
	}

	unsigned GeneratorImplementation::newTree(const glm::vec3& scenePosition) {
		checkIfRunningException();
		if (mTrees.empty()) {
			// Bounding box base is moved by 1/200 down 
			// (so multiple tree roots on not by eye visible bended surface (triangle) are still inside (and not below bounding box)
			auto moveDownPos = scenePosition;
			moveDownPos.y -= mEnvironment.scene->sceneModelRatio();
			mEnvironment.scene->sceneBasePos(moveDownPos);
		}
		const auto id = mNewTreeId++;
		mTrees.emplace(id, std::make_unique<TreeImplementation>(id, scenePosition, mEnvironment));
		return id;
	}

	void GeneratorImplementation::deleteTree(const unsigned Id) {
		checkIfRunningException();
		const auto findIt = mTrees.find(Id);
		if (findIt != std::end(mTrees)) {
			if (findIt->second->nodesCount() != 0) { findIt->second->clearModel(true); }
			mTrees.erase(findIt);
			if (mTrees.empty()) {
				mEnvironment.scene->noBoundingBox();
				std::lock_guard<std::mutex> lock{mStateLock};
				mEnvironmentLocked = false;
			}
		}
	}

	void GeneratorImplementation::deleteAllTrees() {
		checkIfRunningException();
		if (!mTrees.empty()) {
			mTrees.clear();
			mEnvironment.shadow->clear();
			mEnvironment.space->clear();
			mEnvironment.scene->noBoundingBox();
		}
		mNewTreeId = 0;
		std::lock_guard<std::mutex> lock{mStateLock};
		mEnvironmentLocked = false;
	}

	void GeneratorImplementation::clearCurrentModels() {
		checkIfRunningException();
		clearCurrentModelsInternal(true);
	}

	Tree& GeneratorImplementation::tree(const unsigned Id) const {
		const auto findIt = mTrees.find(Id);
		if (findIt == std::end(mTrees)) {
			throw std::invalid_argument{"Tried to acces nonexistent tree! (Invalid Tree Id)"};
		}
		return *findIt->second;
	}

	void GeneratorImplementation::runIterations(const size_t count) {
		checkIfRunningException();
		if (count == 0) { return; }

		{
			std::lock_guard<std::mutex> lock{mStateLock};
			mEnvironmentLocked = true;
			mIsRunning = true;
		}

		for (auto&& idTreePair : mTrees) { idTreePair.second->initModel(); }
		for (int i = 0; i < count; ++i) {
			runIteration();
			if (isCanceled()) {
				clearCurrentModelsInternal(false); // Clear all currently done trees
				break;
			}
		}

		std::lock_guard<std::mutex> lock{mStateLock};
		mIsRunning = false;
		mEnvironmentLocked = false;
		mIsCancelled = false;
	}

	void GeneratorImplementation::cancelIterations() {
		std::lock_guard<std::mutex> lock{mStateLock};
		if (mIsRunning) { mIsCancelled = true; }
	}

	bool GeneratorImplementation::boundingBoxDefined() const { return mEnvironment.scene->boundingBoxDefined(); }

	std::tuple<glm::vec3, glm::vec3> GeneratorImplementation::modelBounds() const {
		return {glm::vec3{0.f}, Const::BOUNDING_BOX_SIDES};
	}

	std::tuple<glm::vec3, glm::vec3> GeneratorImplementation::sceneBounds() const {
		return mEnvironment.scene->sceneBounds();
	}

	std::tuple<glm::vec3, glm::vec3> GeneratorImplementation::boundingBoxBounds() const {
		return mEnvironment.scene->boundingBoxSceneBounds();
	}

	std::tuple<bool, glm::vec3> GeneratorImplementation::tryGetRootPosition(
		const glm::vec3& scenePos, const glm::vec3& direction, const float distance) {
		return mEnvironment.scene->tryGetRootPosition(scenePos, direction, distance);
	}

	void GeneratorImplementation::loadScene(const std::vector<float>& verticesPositions,
	                                        const std::vector<unsigned>& triangleIndices) {
		mEnvironment.scene->loadSceneGeometry(verticesPositions, triangleIndices);
	}

	void GeneratorImplementation::defaultEmptyScene() { mEnvironment.scene->defaultEmptySceneGeometry(); }
	

	// ===========================================================================================================
	// Private methods
	// ===========================================================================================================

	void GeneratorImplementation::clearCurrentModelsInternal(const bool throwIfRunning) {
		if (throwIfRunning) { checkIfRunningException(); }
		for (auto&& idTreePair : mTrees) { idTreePair.second->clearModel(false); }
		mEnvironment.shadow->clear();
		mEnvironment.space->clear();
		std::lock_guard<std::mutex> lock{mStateLock};
		mEnvironmentLocked = false;
	}

	void GeneratorImplementation::runIteration() {
		std::vector<rwTreeImpl> allTrees{};
		allTrees.reserve(mTrees.size());

		for (auto&& idTreePair : mTrees) { allTrees.push_back(*idTreePair.second); }

		std::vector<rwTreeImpl> allActiveTrees{};
		std::vector<rwTreeImpl> treesWithResources{};

		for (auto&& rwTree : allTrees) { rwTree.get().applyEnvironmentResources(); }

		if (isCanceled()) { return; }

		for (auto&& rwTree : allTrees) {
			if (rwTree.get().hasActiveBuds()) {
				allActiveTrees.push_back(rwTree);
				treesWithResources.push_back(rwTree);
			}
		}

		while (!treesWithResources.empty()) {
			if (isCanceled()) { return; }

			for (auto&& rwTree : treesWithResources) { rwTree.get().pushBudsToSpace(); }
			mEnvironment.space->run();
			for (auto&& rwTree : treesWithResources) { rwTree.get().handleSpaceCalculationResults(); }

			for (auto&& rwTree : treesWithResources) { rwTree.get().growShootNode(); }

			treesWithResources.erase(std::remove_if(std::begin(treesWithResources), std::end(treesWithResources),
				[](TreeImplementation& tree) {
					return !tree.hasActiveBuds();
				}), std::end(treesWithResources));
		}

		if (isCanceled()) { return; }

		for (auto&& rwTree : allActiveTrees) {
			rwTree.get().shedBranches();
			rwTree.get().collectValidShootBuds();
		}
	}

	void GeneratorImplementation::checkEnvironmentLockException() {
		const bool isLocked = isEnvironmentLocked();
		if (isLocked) {
			throw std::logic_error{"Tried to set environment properties during tree growth!"};
		}
	}

	void GeneratorImplementation::checkIfRunningException() {
		const bool runs = isRunning();
		if (runs) {
			throw std::logic_error{"Tried to add/delete/access when running algorithm iterations!"};
		}
	}
}
